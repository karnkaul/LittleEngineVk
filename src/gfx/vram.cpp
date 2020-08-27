#include <algorithm>
#include <array>
#include <list>
#include <unordered_map>
#include <vector>
#include <fmt/format.h>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/tasks.hpp>
#include <core/threads.hpp>
#include <kt/async_queue/async_queue.hpp>
#include <gfx/device.hpp>
#include <gfx/vram.hpp>
#include <gfx/renderer_impl.hpp>

namespace le::gfx
{
struct VRAM final
{
};

namespace
{
enum class ResourceType
{
	eBuffer,
	eImage,
	eCOUNT_
};

struct Batch final
{
	struct Stage final
	{
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
std::array<u64, (std::size_t)ResourceType::eCOUNT_> g_allocations;

constexpr vk::DeviceSize operator""_MB(unsigned long long size)
{
	return size << 20;
}

[[maybe_unused]] std::string logCount()
{
	auto [bufferSize, bufferUnit] = utils::friendlySize(g_allocations.at((std::size_t)ResourceType::eBuffer));
	auto const [imageSize, imageUnit] = utils::friendlySize(g_allocations.at((std::size_t)ResourceType::eImage));
	return fmt::format("Buffers: [{:.2f}{}]; Images: [{:.2f}{}]", bufferSize, bufferUnit, imageSize, imageUnit);
}

constexpr std::array<engine::MemRange, 3> const g_stagingReserve = {{{256_MB, 1}, {64_MB, 2}, {8_MB, 4}}};

namespace tfr
{
struct
{
	vk::CommandPool pool;
	std::vector<vk::CommandBuffer> commands;
	std::vector<vk::Fence> fences;
	std::list<Buffer> buffers;

	void scavenge(Batch::Stage const& stage, vk::Fence fence)
	{
		commands.push_back(stage.command);
		buffers.push_back(stage.buffer);
		if (auto search = std::find(fences.begin(), fences.end(), fence) == fences.end())
		{
			g_device.resetFence(fence);
			fences.push_back(fence);
		}
	}
} g_resources;

struct
{
	Batch active;
	std::vector<Batch> submitted;
} g_batches;

struct
{
	threads::Handle thread;
	kt::lockable<> mutex;
} g_sync;

kt::async_queue<std::function<void()>> g_queue;

constexpr vk::DeviceSize ceilPOT(vk::DeviceSize size)
{
	vk::DeviceSize ret = 2;
	while (ret < size)
	{
		ret <<= 1;
	}
	return ret;
}

Buffer createStagingBuffer(vk::DeviceSize size)
{
	BufferInfo info;
	info.size = ceilPOT(size);
	info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	info.queueFlags = QFlag::eGraphics | QFlag::eTransfer;
	info.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
#if defined(LEVK_VKRESOURCE_NAMES)
	static u64 s_nextBufferID = 0;
	info.name = fmt::format("vram-staging-{}", s_nextBufferID++);
#endif
	return vram::createBuffer(info);
}

Buffer nextBuffer(vk::DeviceSize size)
{
	auto lock = g_sync.mutex.lock();
	for (auto iter = g_resources.buffers.begin(); iter != g_resources.buffers.end(); ++iter)
	{
		auto buffer = *iter;
		if (buffer.writeSize >= size)
		{
			g_resources.buffers.erase(iter);
			return buffer;
		}
	}
	return createStagingBuffer(size);
}

vk::CommandBuffer nextCommand()
{
	auto lock = g_sync.mutex.lock();
	if (!g_resources.commands.empty())
	{
		auto ret = g_resources.commands.back();
		g_resources.commands.pop_back();
		return ret;
	}
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = g_resources.pool;
	return g_device.device.allocateCommandBuffers(commandBufferInfo).front();
}

vk::Fence nextFence()
{
	if (!g_resources.fences.empty())
	{
		auto ret = g_resources.fences.back();
		g_resources.fences.pop_back();
		return ret;
	}
	return g_device.createFence(false);
}

Batch::Stage newStage(vk::DeviceSize bufferSize)
{
	Batch::Stage ret;
	ret.command = nextCommand();
	ret.buffer = nextBuffer(bufferSize);
	return ret;
}

void addStage(Batch::Stage&& stage, Batch::Promise&& promise)
{
	auto lock = g_sync.mutex.lock();
	g_batches.active.entries.emplace_back(std::move(stage), std::move(promise));
}

void init(Span<engine::MemRange> stagingReserve)
{
	if (g_resources.pool == vk::CommandPool())
	{
		vk::CommandPoolCreateInfo poolInfo;
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		poolInfo.queueFamilyIndex = g_device.queues.transfer.familyIndex;
		g_resources.pool = g_device.device.createCommandPool(poolInfo);
	}
	g_queue.active(true);
	g_sync.thread = threads::newThread([stagingReserve]() {
		{
			auto lock = g_sync.mutex.lock();
			for (auto const& range : stagingReserve)
			{
				for (auto i = range.count; i > 0; --i)
				{
					g_resources.buffers.push_back(createStagingBuffer(range.size));
				}
			}
		}
		LOG_I("[{}] Transfer thread initialised", s_tName);
		while (auto f = g_queue.pop())
		{
			(*f)();
		}
	});
}

void update()
{
	auto removeDone = [](Batch& batch) -> bool {
		if (g_device.isSignalled(batch.done))
		{
			if (batch.framePad == 0)
			{
				for (auto& [stage, promise] : batch.entries)
				{
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
	if (!g_batches.active.entries.empty())
	{
		std::vector<vk::CommandBuffer> commands;
		commands.reserve(g_batches.active.entries.size());
		g_batches.active.done = nextFence();
		for (auto& [stage, _] : g_batches.active.entries)
		{
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

void deinit()
{
	auto residue = g_queue.clear();
	for (auto& f : residue)
	{
		f();
	}
	threads::join(g_sync.thread);
	LOG_I("[{}] Transfer thread terminated", s_tName);
	g_device.device.waitIdle();
	g_device.destroy(g_resources.pool);
	std::vector<std::shared_ptr<tasks::Handle>> tasks;
	for (auto& fence : g_resources.fences)
	{
		g_device.destroy(fence);
	}
	for (auto& [stage, _] : g_batches.active.entries)
	{
		tasks.push_back(tasks::enqueue([buffer = stage.buffer]() { vram::release(buffer); }, ""));
	}
	for (auto& buffer : g_resources.buffers)
	{
		tasks.push_back(tasks::enqueue([buffer]() { vram::release(buffer); }, ""));
	}
	for (auto& batch : g_batches.submitted)
	{
		for (auto& [stage, _] : batch.entries)
		{
			tasks.push_back(tasks::enqueue([buffer = stage.buffer]() { vram::release(buffer); }, ""));
		}
		g_device.destroy(batch.done);
	}
	g_resources = {};
	g_batches = {};
	tasks::wait(tasks);
}
} // namespace tfr
} // namespace

void vram::init(Span<engine::MemRange> stagingReserve)
{
	if (g_allocator == VmaAllocator())
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.instance = g_instance.instance;
		allocatorInfo.device = g_device.device;
		allocatorInfo.physicalDevice = g_instance.physicalDevice;
		vmaCreateAllocator(&allocatorInfo, &g_allocator);
		std::memset(g_allocations.data(), 0, g_allocations.size() * sizeof(u32));
		tfr::init(stagingReserve.extent == 0 ? g_stagingReserve : stagingReserve);
	}
	LOG_I("[{}] initialised", s_tName);
	return;
}

void vram::deinit()
{
	tfr::deinit();
	bool bErr = false;
	for (auto val : g_allocations)
	{
		ASSERT(val == 0, "Allocations pending release!");
		bErr = val != 0;
	}
	LOGIF_E(bErr, "vram::deinit() => Allocations pending release!");
	if (g_allocator != VmaAllocator())
	{
		vmaDestroyAllocator(g_allocator);
		g_allocator = VmaAllocator();
		LOG_I("[{}] deinitialised", s_tName);
	}
	return;
}

void vram::update()
{
	tfr::update();
}

Buffer vram::createBuffer(BufferInfo const& info, [[maybe_unused]] bool bSilent)
{
	Buffer ret;
#if defined(LEVK_VKRESOURCE_NAMES)
	ASSERT(!info.name.empty(), "Unnamed buffer!");
	ret.name = info.name;
#endif
	vk::BufferCreateInfo bufferInfo;
	ret.writeSize = bufferInfo.size = info.size;
	bufferInfo.usage = info.usage;
	auto const queues = g_device.uniqueQueues(info.queueFlags);
	bufferInfo.sharingMode = queues.mode;
	bufferInfo.queueFamilyIndexCount = (u32)queues.indices.size();
	bufferInfo.pQueueFamilyIndices = queues.indices.data();
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
	VkBuffer vkBuffer;
	if (vmaCreateBuffer(g_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &ret.handle, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Allocation error");
	}
	ret.buffer = vkBuffer;
	ret.queueFlags = info.queueFlags;
	ret.mode = queues.mode;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	g_allocations.at((std::size_t)ResourceType::eBuffer) += ret.writeSize;
	if (g_VRAM_bLogAllocs)
	{
		if (!bSilent)
		{
			auto [size, unit] = utils::friendlySize(ret.writeSize);
#if defined(LEVK_VKRESOURCE_NAMES)
			LOG(g_VRAM_logLevel, "== [{}] Buffer [{}] allocated: [{:.2f}{}] | {}", s_tName, ret.name, size, unit, logCount());
#else
			LOG(g_VRAM_logLevel, "== [{}] Buffer allocated: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
		}
	}
	return ret;
}

bool vram::write(Buffer& out_buffer, void const* pData, vk::DeviceSize size)
{
	if (out_buffer.info.memory != vk::DeviceMemory() && out_buffer.buffer != vk::Buffer())
	{
		if (size == 0)
		{
			size = out_buffer.writeSize;
		}
		if (mapMemory(out_buffer))
		{
			std::memcpy(out_buffer.pMap, pData, size);
		}
		return true;
	}
	return false;
}

bool vram::mapMemory(Buffer& out_buffer)
{
	if (!out_buffer.pMap && out_buffer.writeSize > 0)
	{
		vmaMapMemory(g_allocator, out_buffer.handle, &out_buffer.pMap);
		return true;
	}
	return out_buffer.pMap != nullptr;
}

void vram::unmapMemory(Buffer& out_buffer)
{
	if (out_buffer.pMap)
	{
		vmaUnmapMemory(g_allocator, out_buffer.handle);
		out_buffer.pMap = nullptr;
	}
}

std::future<void> vram::copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = src.writeSize;
	}
#if defined(LEVK_DEBUG)
	auto const uq = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
	ASSERT((uq.indices.size() == 1 || out_dst.mode == vk::SharingMode::eConcurrent), "Exclusive queues!");
#endif
	bool const bQueueFlags = src.queueFlags.isSet(QFlag::eTransfer) && out_dst.queueFlags.isSet(QFlag::eTransfer);
	bool const bSizes = out_dst.writeSize >= size;
	ASSERT(bQueueFlags, "Invalid queue flags!");
	ASSERT(bSizes, "Invalid buffer sizes!");
	if (!bQueueFlags)
	{
		LOG_E("[{}] Source/destination buffers missing QFlag::eTransfer!", s_tName);
		return {};
	}
	if (!bSizes)
	{
		LOG_E("[{}] Source buffer is larger than destination buffer!", s_tName);
		return {};
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

std::future<void> vram::stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = out_deviceBuffer.writeSize;
	}
#if defined(LEVK_DEBUG)
	auto const uq = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
	ASSERT((uq.indices.size() == 1 || out_deviceBuffer.mode == vk::SharingMode::eConcurrent), "Exclusive queues!");
#endif
	bool const bQueueFlags = out_deviceBuffer.queueFlags.isSet(QFlag::eTransfer);
	ASSERT(bQueueFlags, "Invalid queue flags!");
	if (!bQueueFlags)
	{
		LOG_E("[{}] Invalid queue flags on source buffer!", s_tName);
		return {};
	}
	auto promise = std::make_shared<Batch::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, &out_deviceBuffer, pData, size]() mutable {
		auto stage = tfr::newStage(size);
		if (write(stage.buffer, pData, size))
		{
			vk::CommandBufferBeginInfo beginInfo;
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			stage.command.begin(beginInfo);
			vk::BufferCopy copyRegion;
			copyRegion.size = size;
			stage.command.copyBuffer(stage.buffer.buffer, out_deviceBuffer.buffer, copyRegion);
			stage.command.end();
			tfr::addStage(std::move(stage), std::move(promise));
		}
		else
		{
			LOG_E("[{}] Error staging data!", s_tName);
			promise->set_value();
		}
	};
	tfr::g_queue.push(std::move(f));
	return ret;
}

Image vram::createImage(ImageInfo const& info)
{
	Image ret;
#if defined(LEVK_VKRESOURCE_NAMES)
	ASSERT(!info.name.empty(), "Unnamed buffer!");
	ret.name = info.name;
#endif
	vk::ImageCreateInfo imageInfo = info.createInfo;
	auto const queues = g_device.uniqueQueues(info.queueFlags);
	imageInfo.sharingMode = queues.mode;
	imageInfo.queueFamilyIndexCount = (u32)queues.indices.size();
	imageInfo.pQueueFamilyIndices = queues.indices.data();
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = info.vmaUsage;
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (vmaCreateImage(g_allocator, &vkImageInfo, &allocInfo, &vkImage, &ret.handle, nullptr) != VK_SUCCESS)
	{
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
	ret.mode = queues.mode;
	g_allocations.at((std::size_t)ResourceType::eImage) += ret.allocatedSize;
	if (g_VRAM_bLogAllocs)
	{
		auto [size, unit] = utils::friendlySize(ret.allocatedSize);
#if defined(LEVK_VKRESOURCE_NAMES)
		LOG(g_VRAM_logLevel, "== [{}] Image [{}] allocated: [{:.2f}{}] | {}", s_tName, ret.name, size, unit, logCount());
#else
		LOG(g_VRAM_logLevel, "== [{}] Image allocated: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
	}
	return ret;
}

void vram::release(Buffer buffer, [[maybe_unused]] bool bSilent)
{
	unmapMemory(buffer);
	if (buffer.buffer != vk::Buffer())
	{
		vmaDestroyBuffer(g_allocator, buffer.buffer, buffer.handle);
		g_allocations.at((std::size_t)ResourceType::eBuffer) -= buffer.writeSize;
		if (g_VRAM_bLogAllocs)
		{
			if (!bSilent)
			{
				if (buffer.info.actualSize)
				{
					auto [size, unit] = utils::friendlySize(buffer.writeSize);
#if defined(LEVK_VKRESOURCE_NAMES)
					LOG(g_VRAM_logLevel, "-- [{}] Buffer [{}] released: [{:.2f}{}] | {}", s_tName, buffer.name, size, unit, logCount());
#else
					LOG(g_VRAM_logLevel, "-- [{}] Buffer released: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
				}
			}
		}
	}
	return;
}

std::future<void> vram::copy(Span<Span<u8>> pixelsArr, Image const& dst, LayoutTransition layouts)
{
	std::size_t imgSize = 0;
	std::size_t layerSize = 0;
	for (auto pixels : pixelsArr)
	{
		ASSERT(layerSize == 0 || layerSize == pixels.extent, "Invalid image data!");
		layerSize = pixels.extent;
		imgSize += layerSize;
	}
	ASSERT(layerSize > 0 && imgSize > 0, "Invalid image data!");
	[[maybe_unused]] auto const uq = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
	ASSERT((uq.indices.size() == 1 || dst.mode == vk::SharingMode::eConcurrent), "Exclusive queues!");
	auto promise = std::make_shared<Batch::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, pixelsArr, &dst, layouts, imgSize, layerSize]() mutable {
		auto stage = tfr::newStage(imgSize);
		[[maybe_unused]] bool const bResult = mapMemory(stage.buffer);
		ASSERT(bResult, "Memory map failed");
		u32 layerIdx = 0;
		u32 const layerCount = (u32)pixelsArr.extent;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto pixels : pixelsArr)
		{
			auto const offset = layerIdx * layerSize;
			void* pStart = (u8*)stage.buffer.pMap + offset;
			std::memcpy(pStart, pixels.pData, pixels.extent);
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

void vram::release(Image image)
{
	if (image.image != vk::Image())
	{
		vmaDestroyImage(g_allocator, image.image, image.handle);
		g_allocations.at((std::size_t)ResourceType::eImage) -= image.allocatedSize;
		if (g_VRAM_bLogAllocs)
		{
			if (image.info.actualSize > 0)
			{
				auto [size, unit] = utils::friendlySize(image.allocatedSize);
#if defined(LEVK_VKRESOURCE_NAMES)
				LOG(g_VRAM_logLevel, "-- [{}] Image [{}] released: [{:.2f}{}] | {}", s_tName, image.name, size, unit, logCount());
#else
				LOG(g_VRAM_logLevel, "-- [{}] Image released: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
			}
		}
	}
	return;
}
} // namespace le::gfx
