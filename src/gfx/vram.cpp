#include <algorithm>
#include <array>
#include <atomic>
#include <list>
#include <unordered_map>
#include <vector>
#include <fmt/format.h>
#include <core/ensure.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <gfx/device.hpp>
#include <gfx/render_driver_impl.hpp>
#include <gfx/vram.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le::gfx {
struct VRAM final {};

namespace {
enum class ResourceType { eBuffer, eImage, eCOUNT_ };

struct Batch final {
	struct Stage final {
		Buffer buffer;
		vk::CommandBuffer command;
	};

	using Promise = std::shared_ptr<std::promise<void>>;
	using Entry = std::pair<Stage, Promise>;

	std::vector<Entry> entries;
	vk::Fence done;
	u8 framePad = 0;
};

std::string const s_tName = utils::tName<VRAM>();
std::array<std::atomic<u64>, (std::size_t)ResourceType::eCOUNT_> g_allocations;

constexpr vk::DeviceSize operator""_MB(unsigned long long size) {
	return size << 20;
}

[[maybe_unused]] std::string logCount() {
	auto [bufferSize, bufferUnit] = utils::friendlySize(g_allocations[(std::size_t)ResourceType::eBuffer]);
	auto const [imageSize, imageUnit] = utils::friendlySize(g_allocations[(std::size_t)ResourceType::eImage]);
	return fmt::format("Buffers: [{:.2f}{}]; Images: [{:.2f}{}]", bufferSize, bufferUnit, imageSize, imageUnit);
}

constexpr std::array<engine::MemRange, 3> const g_stagingReserve = {{{256_MB, 1}, {64_MB, 2}, {8_MB, 4}}};

namespace tfr {
struct {
	vk::CommandPool pool;
	std::vector<vk::CommandBuffer> commands;
	std::vector<vk::Fence> fences;
	std::list<Buffer> buffers;

	void scavenge(Batch::Stage const& stage, vk::Fence fence) {
		commands.push_back(stage.command);
		buffers.push_back(stage.buffer);
		if (std::find(fences.begin(), fences.end(), fence) == fences.end()) {
			g_device.resetFence(fence);
			fences.push_back(fence);
		}
	}
} g_resources;

struct {
	Batch active;
	std::vector<Batch> submitted;
} g_batches;

struct {
	std::optional<threads::TScoped> thread;
	kt::lockable_t<> mutex;
} g_sync;

kt::async_queue<std::function<void()>> g_queue;

constexpr vk::DeviceSize ceilPOT(vk::DeviceSize size) {
	vk::DeviceSize ret = 2;
	while (ret < size) {
		ret <<= 1;
	}
	return ret;
}

Buffer createStagingBuffer(vk::DeviceSize size) {
	BufferInfo info;
	info.size = ceilPOT(size);
	info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	info.queueFlags = QFlags(QFlag::eGraphics) | QFlag::eTransfer;
	info.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
#if defined(LEVK_VKRESOURCE_NAMES)
	static u64 s_nextBufferID = 0;
	info.name = fmt::format("vram-staging-{}", s_nextBufferID++);
#endif
	return vram::createBuffer(info);
}

Buffer nextBuffer(vk::DeviceSize size) {
	auto lock = g_sync.mutex.lock();
	for (auto iter = g_resources.buffers.begin(); iter != g_resources.buffers.end(); ++iter) {
		auto buffer = *iter;
		if (buffer.writeSize >= size) {
			g_resources.buffers.erase(iter);
			return buffer;
		}
	}
	return createStagingBuffer(size);
}

vk::CommandBuffer nextCommand() {
	auto lock = g_sync.mutex.lock();
	if (!g_resources.commands.empty()) {
		auto ret = g_resources.commands.back();
		g_resources.commands.pop_back();
		return ret;
	}
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = g_resources.pool;
	return g_device.device.allocateCommandBuffers(commandBufferInfo).front();
}

vk::Fence nextFence() {
	if (!g_resources.fences.empty()) {
		auto ret = g_resources.fences.back();
		g_resources.fences.pop_back();
		return ret;
	}
	return g_device.createFence(false);
}

Batch::Stage newStage(vk::DeviceSize bufferSize) {
	Batch::Stage ret;
	ret.command = nextCommand();
	ret.buffer = nextBuffer(bufferSize);
	return ret;
}

void addStage(Batch::Stage&& stage, Batch::Promise&& promise) {
	auto lock = g_sync.mutex.lock();
	g_batches.active.entries.emplace_back(std::move(stage), std::move(promise));
}

void init(View<engine::MemRange> stagingReserve) {
	if (g_resources.pool == vk::CommandPool()) {
		vk::CommandPoolCreateInfo poolInfo;
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		poolInfo.queueFamilyIndex = g_device.queues.transfer.familyIndex;
		g_resources.pool = g_device.device.createCommandPool(poolInfo);
	}
	g_queue.active(true);
	g_sync.thread = threads::newThread([stagingReserve]() {
		{
			auto lock = g_sync.mutex.lock();
			for (auto const& range : stagingReserve) {
				for (auto i = range.count; i > 0; --i) {
					g_resources.buffers.push_back(createStagingBuffer(range.size));
				}
			}
		}
		logI("[{}] Transfer thread initialised", s_tName);
		while (auto f = g_queue.pop()) {
			(*f)();
		}
	});
}

void update() {
	auto removeDone = [](Batch& batch) -> bool {
		if (g_device.isSignalled(batch.done)) {
			if (batch.framePad == 0) {
				for (auto& [stage, promise] : batch.entries) {
					promise->set_value();
					tfr::g_resources.scavenge(stage, batch.done);
				}
				return true;
			}
			--batch.framePad;
		}
		return false;
	};
	auto lock = g_sync.mutex.lock();
	g_batches.submitted.erase(std::remove_if(g_batches.submitted.begin(), g_batches.submitted.end(), removeDone), g_batches.submitted.end());
	if (!g_batches.active.entries.empty()) {
		std::vector<vk::CommandBuffer> commands;
		commands.reserve(g_batches.active.entries.size());
		g_batches.active.done = nextFence();
		for (auto& [stage, _] : g_batches.active.entries) {
			commands.push_back(stage.command);
		}
		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = (u32)commands.size();
		submitInfo.pCommandBuffers = commands.data();
		g_device.queues.transfer.queue.submit(submitInfo, g_batches.active.done);
		g_batches.submitted.push_back(std::move(g_batches.active));
	}
	g_batches.active = {};
}

void deinit() {
	auto residue = g_queue.clear();
	for (auto& f : residue) {
		f();
	}
	g_sync.thread = {};
	logI("[{}] Transfer thread terminated", s_tName);
	g_device.waitIdle();
	g_device.destroy(g_resources.pool);
	for (auto& fence : g_resources.fences) {
		g_device.destroy(fence);
	}
	for (auto& [stage, _] : g_batches.active.entries) {
		vram::release(stage.buffer);
	}
	for (auto& buffer : g_resources.buffers) {
		vram::release(buffer);
	}
	for (auto& batch : g_batches.submitted) {
		for (auto& [stage, _] : batch.entries) {
			vram::release(stage.buffer);
		}
		g_device.destroy(batch.done);
	}
	g_resources = {};
	g_batches = {};
}
} // namespace tfr
} // namespace

vk::SharingMode QShare::operator()(QFlags queues) const {
	auto const indices = g_device.queueIndices(queues);
	return indices.size() == 1 ? vk::SharingMode::eExclusive : desired;
}

void vram::init(View<engine::MemRange> stagingReserve) {
	if (g_allocator == VmaAllocator()) {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.instance = g_instance.instance;
		allocatorInfo.device = g_device.device;
		allocatorInfo.physicalDevice = g_instance.physicalDevice;
		vmaCreateAllocator(&allocatorInfo, &g_allocator);
		for (auto& x : g_allocations) {
			x.store(0);
		}
		tfr::init(stagingReserve.size() == 0 ? g_stagingReserve : stagingReserve);
	}
	logI("[{}] initialised", s_tName);
	return;
}

void vram::deinit() {
	tfr::deinit();
	bool bErr = false;
	for (auto& val : g_allocations) {
		ENSURE(val.load() == 0, "Allocations pending release!");
		bErr = val != 0;
	}
	logE_if(bErr, "vram::deinit() => Allocations pending release!");
	if (g_allocator != VmaAllocator()) {
		vmaDestroyAllocator(g_allocator);
		g_allocator = VmaAllocator();
		logI("[{}] deinitialised", s_tName);
	}
	return;
}

void vram::update() {
	tfr::update();
}

Buffer vram::createBuffer(BufferInfo const& info, [[maybe_unused]] bool bSilent) {
	Buffer ret;
#if defined(LEVK_VKRESOURCE_NAMES)
	ENSURE(!info.name.empty(), "Unnamed buffer!");
	ret.name = info.name;
#endif
	vk::BufferCreateInfo bufferInfo;
	ret.writeSize = bufferInfo.size = info.size;
	bufferInfo.usage = info.usage;
	auto const indices = g_device.queueIndices(info.queueFlags);
	bufferInfo.sharingMode = info.share(info.queueFlags);
	bufferInfo.queueFamilyIndexCount = (u32)indices.size();
	bufferInfo.pQueueFamilyIndices = indices.data();
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
	VkBuffer vkBuffer;
	if (vmaCreateBuffer(g_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &ret.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	ret.buffer = vkBuffer;
	ret.queueFlags = info.queueFlags;
	ret.mode = bufferInfo.sharingMode;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	g_allocations[(std::size_t)ResourceType::eBuffer].fetch_add(ret.writeSize);
	if (g_VRAM_bLogAllocs) {
		if (!bSilent) {
			auto [size, unit] = utils::friendlySize(ret.writeSize);
#if defined(LEVK_VKRESOURCE_NAMES)
			dl::log(g_VRAM_logLevel, "== [{}] Buffer [{}] allocated: [{:.2f}{}] | {}", s_tName, ret.name, size, unit, logCount());
#else
			dl::log(g_VRAM_logLevel, "== [{}] Buffer allocated: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
		}
	}
	return ret;
}

bool vram::write(Buffer& out_buffer, void const* pData, vk::DeviceSize size) {
	if (out_buffer.info.memory != vk::DeviceMemory() && out_buffer.buffer != vk::Buffer()) {
		if (size == 0) {
			size = out_buffer.writeSize;
		}
		if (mapMemory(out_buffer)) {
			std::memcpy(out_buffer.pMap, pData, size);
		}
		return true;
	}
	return false;
}

bool vram::mapMemory(Buffer& out_buffer) {
	if (!out_buffer.pMap && out_buffer.writeSize > 0) {
		vmaMapMemory(g_allocator, out_buffer.handle, &out_buffer.pMap);
		return true;
	}
	return out_buffer.pMap != nullptr;
}

void vram::unmapMemory(Buffer& out_buffer) {
	if (out_buffer.pMap) {
		vmaUnmapMemory(g_allocator, out_buffer.handle);
		out_buffer.pMap = nullptr;
	}
}

std::future<void> vram::copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size) {
	if (size == 0) {
		size = src.writeSize;
	}
	[[maybe_unused]] auto const& sq = src.queueFlags;
	[[maybe_unused]] auto const& dq = out_dst.queueFlags;
	[[maybe_unused]] bool const bReady = sq.test(QFlag::eTransfer) && dq.test(QFlag::eTransfer);
	ENSURE(bReady, "Transfer flag not set!");
	bool const bSizes = out_dst.writeSize >= size;
	ENSURE(bSizes, "Invalid buffer sizes!");
	if (!bReady) {
		logE("[{}] Source/destination buffers missing QFlag::eTransfer!", s_tName);
		return {};
	}
	if (!bSizes) {
		logE("[{}] Source buffer is larger than destination buffer!", s_tName);
		return {};
	}
	[[maybe_unused]] auto const indices = g_device.queueIndices(QFlags(QFlag::eGraphics) | QFlag::eTransfer);
	if (indices.size() > 1) {
		ENSURE(sq.test() <= 1 || src.mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
		ENSURE(dq.test() <= 1 || out_dst.mode == vk::SharingMode::eConcurrent, "Unsupported sharing mode!");
	}
	auto promise = std::make_shared<Batch::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, &src, &out_dst, size]() mutable {
		auto stage = tfr::newStage(size);
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		stage.command.begin(beginInfo);
		vk::BufferCopy copyRegion;
		copyRegion.size = size;
		stage.command.copyBuffer(src.buffer, out_dst.buffer, copyRegion);
		stage.command.end();
		tfr::addStage(std::move(stage), std::move(promise));
	};
	tfr::g_queue.push(std::move(f));
	return ret;
}

std::future<void> vram::stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size) {
	if (size == 0) {
		size = out_deviceBuffer.writeSize;
	}
	auto const indices = g_device.queueIndices(QFlags(QFlag::eGraphics) | QFlag::eTransfer);
	ENSURE(indices.size() == 1 || out_deviceBuffer.mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	bool const bQueueFlags = out_deviceBuffer.queueFlags.test(QFlag::eTransfer);
	ENSURE(bQueueFlags, "Invalid queue flags!");
	if (!bQueueFlags) {
		logE("[{}] Invalid queue flags on source buffer!", s_tName);
		return {};
	}
	auto promise = std::make_shared<Batch::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, &out_deviceBuffer, pData, size]() mutable {
		auto stage = tfr::newStage(size);
		if (write(stage.buffer, pData, size)) {
			vk::CommandBufferBeginInfo beginInfo;
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			stage.command.begin(beginInfo);
			vk::BufferCopy copyRegion;
			copyRegion.size = size;
			stage.command.copyBuffer(stage.buffer.buffer, out_deviceBuffer.buffer, copyRegion);
			stage.command.end();
			tfr::addStage(std::move(stage), std::move(promise));
		} else {
			logE("[{}] Error staging data!", s_tName);
			promise->set_value();
		}
	};
	tfr::g_queue.push(std::move(f));
	return ret;
}

Image vram::createImage(ImageInfo const& info) {
	Image ret;
#if defined(LEVK_VKRESOURCE_NAMES)
	ENSURE(!info.name.empty(), "Unnamed buffer!");
	ret.name = info.name;
#endif
	vk::ImageCreateInfo imageInfo = info.createInfo;
	auto const indices = g_device.queueIndices(info.queueFlags);
	imageInfo.sharingMode = info.share(info.queueFlags);
	imageInfo.queueFamilyIndexCount = (u32)indices.size();
	imageInfo.pQueueFamilyIndices = indices.data();
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = info.vmaUsage;
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (vmaCreateImage(g_allocator, &vkImageInfo, &allocInfo, &vkImage, &ret.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	ret.extent = info.createInfo.extent;
	ret.image = vkImage;
	auto const requirements = g_device.device.getImageMemoryRequirements(ret.image);
	ret.queueFlags = info.queueFlags;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	ret.allocatedSize = requirements.size;
	ret.mode = imageInfo.sharingMode;
	g_allocations[(std::size_t)ResourceType::eImage].fetch_add(ret.allocatedSize);
	if (g_VRAM_bLogAllocs) {
		auto [size, unit] = utils::friendlySize(ret.allocatedSize);
#if defined(LEVK_VKRESOURCE_NAMES)
		dl::log(g_VRAM_logLevel, "== [{}] Image [{}] allocated: [{:.2f}{}] | {}", s_tName, ret.name, size, unit, logCount());
#else
		dl::log(g_VRAM_logLevel, "== [{}] Image allocated: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
	}
	return ret;
}

void vram::release(Buffer buffer, [[maybe_unused]] bool bSilent) {
	unmapMemory(buffer);
	if (buffer.buffer != vk::Buffer()) {
		vmaDestroyBuffer(g_allocator, buffer.buffer, buffer.handle);
		g_allocations[(std::size_t)ResourceType::eBuffer].fetch_sub(buffer.writeSize);
		if (g_VRAM_bLogAllocs) {
			if (!bSilent) {
				if (buffer.info.actualSize) {
					auto [size, unit] = utils::friendlySize(buffer.writeSize);
#if defined(LEVK_VKRESOURCE_NAMES)
					dl::log(g_VRAM_logLevel, "-- [{}] Buffer [{}] released: [{:.2f}{}] | {}", s_tName, buffer.name, size, unit, logCount());
#else
					dl::log(g_VRAM_logLevel, "-- [{}] Buffer released: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
				}
			}
		}
	}
	return;
}

std::future<void> vram::copy(View<View<u8>> pixelsArr, Image const& dst, LayoutTransition layouts) {
	std::size_t imgSize = 0;
	std::size_t layerSize = 0;
	for (auto pixels : pixelsArr) {
		ENSURE(layerSize == 0 || layerSize == pixels.size(), "Invalid image data!");
		layerSize = pixels.size();
		imgSize += layerSize;
	}
	ENSURE(layerSize > 0 && imgSize > 0, "Invalid image data!");
	[[maybe_unused]] auto const indices = g_device.queueIndices(QFlags(QFlag::eGraphics) | QFlag::eTransfer);
	ENSURE(indices.size() == 1 || dst.mode == vk::SharingMode::eConcurrent, "Exclusive queues!");
	auto promise = std::make_shared<Batch::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, pixelsArr, &dst, layouts, imgSize, layerSize]() mutable {
		auto stage = tfr::newStage(imgSize);
		[[maybe_unused]] bool const bResult = mapMemory(stage.buffer);
		ENSURE(bResult, "Memory map failed");
		u32 layerIdx = 0;
		u32 const layerCount = (u32)pixelsArr.size();
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto pixels : pixelsArr) {
			auto const offset = layerIdx * layerSize;
			void* pStart = (u8*)stage.buffer.pMap + offset;
			std::memcpy(pStart, pixels.data(), pixels.size());
			vk::BufferImageCopy copyRegion;
			copyRegion.bufferOffset = offset;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = (u32)layerIdx;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageOffset = vk::Offset3D(0, 0, 0);
			copyRegion.imageExtent = dst.extent;
			copyRegions.push_back(std::move(copyRegion));
			++layerIdx;
		}
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		stage.command.begin(beginInfo);
		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = layouts.pre;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = dst.image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		using vkstg = vk::PipelineStageFlagBits;
		stage.command.pipelineBarrier(vkstg::eTopOfPipe, vkstg::eTransfer, {}, {}, {}, barrier);
		stage.command.copyBufferToImage(stage.buffer.buffer, dst.image, vk::ImageLayout::eTransferDstOptimal, copyRegions);
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = layouts.post;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = dst.image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = {};
		stage.command.pipelineBarrier(vkstg::eTransfer, vkstg::eBottomOfPipe, {}, {}, {}, barrier);
		stage.command.end();
		tfr::addStage(std::move(stage), std::move(promise));
	};
	tfr::g_queue.push(std::move(f));
	return ret;
}

void vram::release(Image image) {
	if (image.image != vk::Image()) {
		vmaDestroyImage(g_allocator, image.image, image.handle);
		g_allocations[(std::size_t)ResourceType::eImage].fetch_sub(image.allocatedSize);
		if (g_VRAM_bLogAllocs) {
			if (image.info.actualSize > 0) {
				auto [size, unit] = utils::friendlySize(image.allocatedSize);
#if defined(LEVK_VKRESOURCE_NAMES)
				dl::log(g_VRAM_logLevel, "-- [{}] Image [{}] released: [{:.2f}{}] | {}", s_tName, image.name, size, unit, logCount());
#else
				dl::log(g_VRAM_logLevel, "-- [{}] Image released: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
			}
		}
	}
	return;
}
} // namespace le::gfx
