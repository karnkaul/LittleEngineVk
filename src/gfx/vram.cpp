#include <algorithm>
#include <array>
#include <deque>
#include <unordered_map>
#include <thread>
#include <fmt/format.h>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/utils.hpp>
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

struct Stage final
{
	struct Command final
	{
		Buffer buffer;
		vk::CommandBuffer commandBuffer;
		std::promise<void> promise;
		size_t bufferID = 0;
	};

	std::deque<Command> commands;

	Command& next(vk::DeviceSize bufferSize);
};

struct Submit final
{
	std::deque<Stage> stages;
	vk::Fence done;
};

std::string const s_tName = utils::tName<VRAM>();
std::array<u64, (size_t)ResourceType::eCOUNT_> g_allocations;

std::unordered_map<std::thread::id, Stage> g_active;
std::unordered_map<std::thread::id, vk::CommandPool> g_pools;
std::deque<Submit> g_submitted;

std::mutex g_mutex;

Buffer createStagingBuffer(vk::DeviceSize size)
{
	BufferInfo info;
	info.size = size;
	info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	info.queueFlags = QFlag::eGraphics | QFlag::eTransfer;
	info.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
#if defined(LEVK_VKRESOURCE_NAMES)
	info.name = fmt::format("vram-staging-{}", threads::thisThreadID());
#endif
	return vram::createBuffer(info);
}

Stage::Command newCommand(vk::DeviceSize bufferSize)
{
	auto const id = std::this_thread::get_id();
	std::unique_lock<std::mutex> lock(g_mutex);
	auto& pool = g_pools[id];
	lock.unlock();
	if (pool == vk::CommandPool())
	{
		vk::CommandPoolCreateInfo poolInfo;
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		poolInfo.queueFamilyIndex = g_device.queues.transfer.familyIndex;
		pool = g_device.device.createCommandPool(poolInfo);
		if constexpr (g_VRAM_bLogAllocs)
		{
			LOG(g_VRAM_logLevel, "[{}] Created command pool for thread [{}]", s_tName, threads::thisThreadID());
		}
	}
	Stage::Command ret;
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = pool;
	ret.commandBuffer = g_device.device.allocateCommandBuffers(commandBufferInfo).front();
	ret.buffer = createStagingBuffer(std::max(bufferSize, ret.buffer.writeSize * 2));
	return ret;
}

void addCommand(Stage::Command&& command)
{
	auto const id = std::this_thread::get_id();
	std::scoped_lock<std::mutex> lock(g_mutex);
	auto& stage = g_active[id];
	stage.commands.push_back(std::move(command));
}

[[maybe_unused]] std::string logCount()
{
	auto [bufferSize, bufferUnit] = utils::friendlySize(g_allocations.at((size_t)ResourceType::eBuffer));
	auto const [imageSize, imageUnit] = utils::friendlySize(g_allocations.at((size_t)ResourceType::eImage));
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
		std::scoped_lock<std::mutex> lock(g_mutex);
		std::memset(g_allocations.data(), 0, g_allocations.size() * sizeof(u32));
	}
	LOG_I("[{}] initialised", s_tName);
	return;
}

void vram::deinit()
{
	g_device.device.waitIdle();
	for (auto& [_, pool] : g_pools)
	{
		g_device.destroy(pool);
	}
	g_pools.clear();
	for (auto& [_, stage] : g_active)
	{
		for (auto& command : stage.commands)
		{
			release(command.buffer);
		}
	}
	g_active.clear();
	for (auto& submit : g_submitted)
	{
		for (auto& stage : submit.stages)
		{
			for (auto& command : stage.commands)
			{
				release(command.buffer);
			}
		}
		g_device.destroy(submit.done);
	}
	g_submitted.clear();
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
	auto removeDone = [](Submit& submit) -> bool {
		if (g_device.isSignalled(submit.done))
		{
			for (auto& stage : submit.stages)
			{
				for (auto& command : stage.commands)
				{
					command.promise.set_value();
					release(command.buffer);
				}
			}
			g_device.destroy(submit.done);
			return true;
		}
		return false;
	};
	g_submitted.erase(std::remove_if(g_submitted.begin(), g_submitted.end(), removeDone), g_submitted.end());
	std::scoped_lock<std::mutex> lock(g_mutex);
	size_t commandCount = 0;
	for (auto& [_, stage] : g_active)
	{
		commandCount += stage.commands.size();
	}
	if (commandCount > 0)
	{
		std::vector<vk::CommandBuffer> buffers;
		buffers.reserve(commandCount);
		Submit submit;
		submit.done = g_device.createFence(false);
		for (auto& [_, stage] : g_active)
		{
			for (auto& command : stage.commands)
			{
				buffers.push_back(command.commandBuffer);
			}
			submit.stages.push_back(std::move(stage));
		}
		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = (u32)commandCount;
		submitInfo.pCommandBuffers = buffers.data();
		g_device.queues.transfer.queue.submit(submitInfo, submit.done);
		g_submitted.push_back(std::move(submit));
	}
	g_active.clear();
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
	std::scoped_lock<std::mutex> lock(g_mutex);
	g_allocations.at((size_t)ResourceType::eBuffer) += ret.writeSize;
	if constexpr (g_VRAM_bLogAllocs)
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
		auto pMem = mapMemory(buffer, size);
		std::memcpy(pMem, pData, size);
		unmapMemory(buffer);
		return true;
	}
	return false;
}

void* vram::mapMemory(Buffer const& buffer, vk::DeviceSize size)
{
	void* pRet = nullptr;
	if (buffer.writeSize > 0)
	{
		if (size == 0)
		{
			size = buffer.writeSize;
		}

		std::scoped_lock<std::mutex> lock(g_mutex);
		vmaMapMemory(g_allocator, buffer.handle, &pRet);
	}
	return pRet;
}

void vram::unmapMemory(Buffer const& buffer)
{
	if (buffer.writeSize > 0)
	{
		std::scoped_lock<std::mutex> lock(g_mutex);
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
	auto command = newCommand(size);
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	command.commandBuffer.begin(beginInfo);
	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	command.commandBuffer.copyBuffer(src.buffer, dst.buffer, copyRegion);
	command.commandBuffer.end();
	auto ret = command.promise.get_future();
	addCommand(std::move(command));
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
		auto ret = command.promise.get_future();
		addCommand(std::move(command));
		return ret;
	}
	LOG_E("[{}] Error staging data!", s_tName);
	return {};
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
	std::scoped_lock<std::mutex> lock(g_mutex);
	g_allocations.at((size_t)ResourceType::eImage) += ret.allocatedSize;
	if constexpr (g_VRAM_bLogAllocs)
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
		std::scoped_lock<std::mutex> lock(g_mutex);
		g_allocations.at((size_t)ResourceType::eBuffer) -= buffer.writeSize;
		if constexpr (g_VRAM_bLogAllocs)
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
	size_t imgSize = 0;
	size_t layerSize = 0;
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
	auto ret = command.promise.get_future();
	addCommand(std::move(command));
	return ret;
}

void vram::release(Image image)
{
	if (image.image != vk::Image())
	{
		vmaDestroyImage(g_allocator, image.image, image.handle);
		std::scoped_lock<std::mutex> lock(g_mutex);
		g_allocations.at((size_t)ResourceType::eImage) -= image.allocatedSize;
		if constexpr (g_VRAM_bLogAllocs)
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
