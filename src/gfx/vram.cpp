#include <algorithm>
#include <array>
#include <deque>
#include <list>
#include <unordered_map>
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

struct Staged final
{
	struct Stage final
	{
		Buffer buffer;
		vk::CommandBuffer commandBuffer;
	};

	using Promise = std::shared_ptr<std::promise<void>>;
	using Entry = std::pair<Stage, Promise>;

	std::deque<Entry> entries;
	vk::Fence done;
	u16 pad = 3;
};

std::string const s_tName = utils::tName<VRAM>();

struct
{
	kt::async_queue<std::function<void()>> queue;
	threads::Handle thread;

	void start()
	{
		queue.active(true);
		thread = threads::newThread([this]() {
			LOGIF_I(g_VRAM_bLogAllocs, "[{}] Transfer thread initialised", s_tName)
			while (auto f = queue.pop())
			{
				(*f)();
			}
		});
	}

	void stop()
	{
		auto r = queue.clear();
		for (auto& f : r)
		{
			f();
		}
		threads::join(thread);
		LOGIF_I(g_VRAM_bLogAllocs, "[{}] Transfer thread terminated", s_tName)
	}

	void push(std::function<void()>&& f)
	{
		queue.push(std::move(f));
	}
} g_queue;

std::array<u64, (std::size_t)ResourceType::eCOUNT_> g_allocations;
[[maybe_unused]] u64 g_nextBufferID = 0;

Staged g_active;
vk::CommandPool g_pool;
std::deque<Staged> g_submitted;
struct
{
	std::list<Buffer> buffers;
	kt::lockable<> mutex;
} g_buffers;

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
	info.name = fmt::format("vram-staging-{}", g_nextBufferID++);
#endif
	return vram::createBuffer(info);
}

struct
{
	vk::DeviceSize const s_1MB = 1024U * 1024U;
	std::vector<vk::DeviceSize> const bufferSizes = {s_1MB * 256, s_1MB * 64, s_1MB * 64, s_1MB * 8, s_1MB * 8, s_1MB * 8, s_1MB * 8};
	threads::Handle thread;

	void start()
	{
		thread = threads::newThread([this]() {
			for (auto size : bufferSizes)
			{
				threads::sleep();
				auto lock = g_buffers.mutex.lock();
				g_buffers.buffers.push_back(createStagingBuffer(size));
			}
		});
	}

	void stop()
	{
		threads::join(thread);
	}
} g_init;

Buffer reuseOrCreateBuffer(vk::DeviceSize size)
{
	auto lock = g_buffers.mutex.lock();
	for (auto iter = g_buffers.buffers.begin(); iter != g_buffers.buffers.end(); ++iter)
	{
		auto buffer = *iter;
		if (buffer.writeSize >= size)
		{
			g_buffers.buffers.erase(iter);
			return buffer;
		}
	}
	return createStagingBuffer(size);
}

Staged::Stage newCommand(vk::DeviceSize bufferSize)
{
	if (g_pool == vk::CommandPool())
	{
		vk::CommandPoolCreateInfo poolInfo;
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		poolInfo.queueFamilyIndex = g_device.queues.transfer.familyIndex;
		g_pool = g_device.device.createCommandPool(poolInfo);
		if (g_VRAM_bLogAllocs)
		{
			LOG(g_VRAM_logLevel, "[{}] Created command pool for thread [{}]", s_tName, threads::thisThreadID());
		}
	}
	Staged::Stage ret;
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = g_pool;
	ret.commandBuffer = g_device.device.allocateCommandBuffers(commandBufferInfo).front();
	ret.buffer = reuseOrCreateBuffer(bufferSize);
	return ret;
}

void addCommand(Staged::Stage&& command, Staged::Promise&& promise)
{
	auto lock = g_buffers.mutex.lock();
	g_active.entries.emplace_back(std::move(command), std::move(promise));
}

[[maybe_unused]] std::string logCount()
{
	auto [bufferSize, bufferUnit] = utils::friendlySize(g_allocations.at((std::size_t)ResourceType::eBuffer));
	auto const [imageSize, imageUnit] = utils::friendlySize(g_allocations.at((std::size_t)ResourceType::eImage));
	return fmt::format("Buffers: [{:.2f}{}]; Images: [{:.2f}{}]", bufferSize, bufferUnit, imageSize, imageUnit);
}
} // namespace

void vram::init()
{
	if (g_allocator == VmaAllocator())
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.instance = g_instance.instance;
		allocatorInfo.device = g_device.device;
		allocatorInfo.physicalDevice = g_instance.physicalDevice;
		vmaCreateAllocator(&allocatorInfo, &g_allocator);
		std::memset(g_allocations.data(), 0, g_allocations.size() * sizeof(u32));
		g_queue.start();
		g_init.start();
	}
	LOG_I("[{}] initialised", s_tName);
	return;
}

void vram::deinit()
{
	g_init.stop();
	g_queue.stop();
	g_device.device.waitIdle();
	g_device.destroy(g_pool);
	std::vector<std::shared_ptr<tasks::Handle>> tasks;
	for (auto& [command, _] : g_active.entries)
	{
		tasks.push_back(tasks::enqueue([buffer = command.buffer]() { release(buffer); }, ""));
	}
	for (auto& buffer : g_buffers.buffers)
	{
		tasks.push_back(tasks::enqueue([buffer]() { release(buffer); }, ""));
	}
	g_active = {};
	for (auto& stage : g_submitted)
	{
		for (auto& [command, _] : stage.entries)
		{
			tasks.push_back(tasks::enqueue([buffer = command.buffer]() { release(buffer); }, ""));
		}
		g_device.destroy(stage.done);
	}
	g_submitted.clear();
	tasks::wait(tasks);
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
	auto removeDone = [](Staged& stage) -> bool {
		if (g_device.isSignalled(stage.done))
		{
			if (stage.pad == 0)
			{
				for (auto& [command, promise] : stage.entries)
				{
					promise->set_value();
					g_buffers.buffers.push_back(command.buffer);
				}
				g_device.destroy(stage.done);
				return true;
			}
			--stage.pad;
		}
		return false;
	};
	auto lock = g_buffers.mutex.lock();
	g_submitted.erase(std::remove_if(g_submitted.begin(), g_submitted.end(), removeDone), g_submitted.end());
	if (!g_active.entries.empty())
	{
		std::vector<vk::CommandBuffer> buffers;
		buffers.reserve(g_active.entries.size());
		g_device.resetOrCreateFence(g_active.done, false);
		for (auto& [command, _] : g_active.entries)
		{
			buffers.push_back(command.commandBuffer);
		}
		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = (u32)buffers.size();
		submitInfo.pCommandBuffers = buffers.data();
		g_device.queues.transfer.queue.submit(submitInfo, g_active.done);
		g_submitted.push_back(std::move(g_active));
	}
	g_active = {};
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

bool vram::write(Buffer const& buffer, void const* pData, vk::DeviceSize size)
{
	if (buffer.info.memory != vk::DeviceMemory() && buffer.buffer != vk::Buffer())
	{
		if (size == 0)
		{
			size = buffer.writeSize;
		}
		auto pMem = mapMemory(buffer);
		std::memcpy(pMem, pData, size);
		unmapMemory(buffer);
		return true;
	}
	return false;
}

void* vram::mapMemory(Buffer const& buffer)
{
	void* pRet = nullptr;
	if (buffer.writeSize > 0)
	{
		vmaMapMemory(g_allocator, buffer.handle, &pRet);
	}
	return pRet;
}

void vram::unmapMemory(Buffer const& buffer)
{
	if (buffer.writeSize > 0)
	{
		vmaUnmapMemory(g_allocator, buffer.handle);
	}
}

std::future<void> vram::copy(Buffer const& src, Buffer const& dst, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = src.writeSize;
	}
#if defined(LEVK_DEBUG)
	auto const uq = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
	ASSERT((uq.indices.size() == 1 || dst.mode == vk::SharingMode::eConcurrent), "Exclusive queues!");
#endif
	bool const bQueueFlags = src.queueFlags.isSet(QFlag::eTransfer) && dst.queueFlags.isSet(QFlag::eTransfer);
	bool const bSizes = dst.writeSize >= size;
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
	auto promise = std::make_shared<Staged::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, &src, &dst, size]() mutable {
		auto command = newCommand(size);
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		command.commandBuffer.begin(beginInfo);
		vk::BufferCopy copyRegion;
		copyRegion.size = size;
		command.commandBuffer.copyBuffer(src.buffer, dst.buffer, copyRegion);
		command.commandBuffer.end();
		addCommand(std::move(command), std::move(promise));
	};
	g_queue.push(std::move(f));
	return ret;
}

std::future<void> vram::stage(Buffer const& deviceBuffer, void const* pData, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = deviceBuffer.writeSize;
	}
#if defined(LEVK_DEBUG)
	auto const uq = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
	ASSERT((uq.indices.size() == 1 || deviceBuffer.mode == vk::SharingMode::eConcurrent), "Exclusive queues!");
#endif
	bool const bQueueFlags = deviceBuffer.queueFlags.isSet(QFlag::eTransfer);
	ASSERT(bQueueFlags, "Invalid queue flags!");
	if (!bQueueFlags)
	{
		LOG_E("[{}] Invalid queue flags on source buffer!", s_tName);
		return {};
	}
	auto promise = std::make_shared<Staged::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, &deviceBuffer, pData, size]() mutable {
		auto command = newCommand(size);
		if (write(command.buffer, pData, size))
		{
			vk::CommandBufferBeginInfo beginInfo;
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			command.commandBuffer.begin(beginInfo);
			vk::BufferCopy copyRegion;
			copyRegion.size = size;
			command.commandBuffer.copyBuffer(command.buffer.buffer, deviceBuffer.buffer, copyRegion);
			command.commandBuffer.end();
			addCommand(std::move(command), std::move(promise));
		}
		else
		{
			LOG_E("[{}] Error staging data!", s_tName);
			promise->set_value();
		}
	};
	g_queue.push(std::move(f));
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
#if defined(LEVK_DEBUG)
	auto const uq = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
	ASSERT((uq.indices.size() == 1 || dst.mode == vk::SharingMode::eConcurrent), "Exclusive queues!");
#endif
	auto promise = std::make_shared<Staged::Promise::element_type>();
	auto ret = promise->get_future();
	auto f = [promise, pixelsArr, &dst, layouts, imgSize, layerSize]() mutable {
		auto command = newCommand(imgSize);
		auto pMem = mapMemory(command.buffer);
		u32 layerIdx = 0;
		u32 const layerCount = (u32)pixelsArr.extent;
		std::vector<vk::BufferImageCopy> copyRegions;
		for (auto pixels : pixelsArr)
		{
			auto const offset = layerIdx * layerSize;
			void* pStart = (u8*)pMem + offset;
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
		unmapMemory(command.buffer);
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		command.commandBuffer.begin(beginInfo);
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
		command.commandBuffer.pipelineBarrier(vkstg::eTopOfPipe, vkstg::eTransfer, {}, {}, {}, barrier);
		command.commandBuffer.copyBufferToImage(command.buffer.buffer, dst.image, vk::ImageLayout::eTransferDstOptimal, copyRegions);
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
		command.commandBuffer.pipelineBarrier(vkstg::eTransfer, vkstg::eBottomOfPipe, {}, {}, {}, barrier);
		command.commandBuffer.end();
		addCommand(std::move(command), std::move(promise));
	};
	g_queue.push(std::move(f));
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
