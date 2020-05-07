#include <array>
#include <deque>
#include <mutex>
#include <thread>
#include <fmt/format.h>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "info.hpp"
#include "utils.hpp"
#include "vram.hpp"

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

struct Transfer final
{
	vk::CommandBuffer commandBuffer;
	vk::Fence done;
};

struct Stage final
{
	Buffer buffer;
	Transfer transfer;
};

std::string const s_tName = utils::tName<VRAM>();
std::array<u64, (size_t)ResourceType::eCOUNT_> g_allocations;

vk::CommandPool g_transferPool;
vk::CommandPool g_graphicsPool;

std::deque<Stage> g_stages;
std::deque<Transfer> g_transfers;
std::mutex g_mutex;

Buffer createStagingBuffer(vk::DeviceSize size)
{
	gfx::BufferInfo info;
	info.size = size;
	info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	info.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
	info.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
#if defined(LEVK_VKRESOURCE_NAMES)
	info.name = "vram-staging";
#endif
	return vram::createBuffer(info);
}

Transfer newTransfer()
{
	Transfer ret;
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = g_transferPool;
	ret.commandBuffer = g_info.device.allocateCommandBuffers(commandBufferInfo).front();
	ret.done = g_info.device.createFence({});
	return ret;
}

Stage& getNextStage(vk::DeviceSize size)
{
	std::unique_lock<std::mutex> lock(g_mutex);
	for (auto& stage : g_stages)
	{
		if (isSignalled(stage.transfer.done))
		{
			g_info.device.resetFences(stage.transfer.done);
			stage.transfer.commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			if (stage.buffer.writeSize < size)
			{
				vram::release(stage.buffer);
				stage.buffer = createStagingBuffer(std::max(size, stage.buffer.writeSize * 2));
			}
			return stage;
		}
	}
	lock.unlock();
	Stage newStage;
	newStage.buffer = createStagingBuffer(size);
	newStage.transfer = newTransfer();
	lock.lock();
	g_stages.push_back(newStage);
	return g_stages.back();
}

Transfer& getNextTransfer()
{
	std::unique_lock<std::mutex> lock(g_mutex);
	for (auto& transfer : g_transfers)
	{
		if (isSignalled(transfer.done))
		{
			g_info.device.resetFences(transfer.done);
			transfer.commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			return transfer;
		}
	}
	g_transfers.push_back(newTransfer());
	return g_transfers.back();
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
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	if (g_allocator == VmaAllocator())
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.instance = g_info.instance;
		allocatorInfo.device = g_info.device;
		allocatorInfo.physicalDevice = g_info.physicalDevice;
		vmaCreateAllocator(&allocatorInfo, &g_allocator);
		std::memset(g_allocations.data(), 0, g_allocations.size() * sizeof(u32));
	}
	if (g_transferPool == vk::CommandPool())
	{
		poolInfo.queueFamilyIndex = g_info.queueFamilyIndices.transfer;
		g_transferPool = g_info.device.createCommandPool(poolInfo);
	}
	if (g_graphicsPool == vk::CommandPool())
	{
		if (g_info.queueFamilyIndices.transfer == g_info.queueFamilyIndices.graphics)
		{
			g_graphicsPool = g_transferPool;
		}
		else
		{
			poolInfo.queueFamilyIndex = g_info.queueFamilyIndices.graphics;
			g_graphicsPool = g_info.device.createCommandPool(poolInfo);
		}
	}
	LOG_I("[{}] initialised", s_tName);
	return;
}

void vram::deinit()
{
	g_info.device.waitIdle();
	std::unique_lock<std::mutex> lock(g_mutex);
	for (auto& stage : g_stages)
	{
		release(stage.buffer);
		vkDestroy(stage.transfer.done);
	}
	for (auto& transfer : g_transfers)
	{
		vkDestroy(transfer.done);
	}
	g_stages.clear();
	g_transfers.clear();
	vkDestroy(g_transferPool, g_graphicsPool);
	g_transferPool = g_graphicsPool = vk::CommandPool();
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
	auto const queues = g_info.uniqueQueues(info.queueFlags);
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
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	g_allocations.at((size_t)ResourceType::eBuffer) += ret.writeSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		if (!bSilent)
		{
			auto [size, unit] = utils::friendlySize(ret.writeSize);
#if defined(LEVK_VKRESOURCE_NAMES)
			LOG_I("== [{}] Buffer [{}] allocated: [{:.2f}{}] | {}", s_tName, ret.name, size, unit, logCount());
#else
			LOG_I("== [{}] Buffer allocated: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
		}
	}
	return ret;
}

bool vram::write(Buffer buffer, void const* pData, vk::DeviceSize size)
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
	if (size == 0)
	{
		size = buffer.writeSize;
	}
	void* pRet;
	vmaMapMemory(g_allocator, buffer.handle, &pRet);
	return pRet;
}

void vram::unmapMemory(Buffer const& buffer)
{
	vmaUnmapMemory(g_allocator, buffer.handle);
}

vk::Fence vram::copy(Buffer const& src, Buffer const& dst, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = src.writeSize;
	}
	bool bQueueFlags = src.queueFlags.isSet(QFlag::eTransfer) && dst.queueFlags.isSet(QFlag::eTransfer);
	bool bSizes = dst.writeSize >= size;
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
	Transfer& ret = getNextTransfer();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	ret.commandBuffer.begin(beginInfo);
	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	ret.commandBuffer.copyBuffer(src.buffer, dst.buffer, copyRegion);
	ret.commandBuffer.end();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &ret.commandBuffer;
	ret.done = g_info.device.createFence({});
	g_info.queues.transfer.submit(submitInfo, ret.done);
	return ret.done;
}

vk::Fence vram::stage(Buffer const& deviceBuffer, void const* pData, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = deviceBuffer.writeSize;
	}
	auto& stage = getNextStage(size);
	if (write(stage.buffer, pData, size))
	{
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		stage.transfer.commandBuffer.begin(beginInfo);
		vk::BufferCopy copyRegion;
		copyRegion.size = size;
		stage.transfer.commandBuffer.copyBuffer(stage.buffer.buffer, deviceBuffer.buffer, copyRegion);
		stage.transfer.commandBuffer.end();
		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &stage.transfer.commandBuffer;
		g_info.queues.transfer.submit(submitInfo, stage.transfer.done);
		return stage.transfer.done;
	}
	LOG_E("[{}] Error staging data!", s_tName);
	return {};
}

vk::Fence vram::copy(ArrayView<u8> pixels, Image const& dst, std::pair<vk::ImageLayout, vk::ImageLayout> layouts)
{
	auto& stage = getNextStage(pixels.extent);
	write(stage.buffer, pixels.pData, pixels.extent);
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	stage.transfer.commandBuffer.begin(beginInfo);
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = layouts.first;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = dst.image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	stage.transfer.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
												 barrier);
	vk::BufferImageCopy copyRegion;
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = vk::Offset3D(0, 0, 0);
	copyRegion.imageExtent = dst.extent;
	stage.transfer.commandBuffer.copyBufferToImage(stage.buffer.buffer, dst.image, vk::ImageLayout::eTransferDstOptimal, copyRegion);
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = layouts.second;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = dst.image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
	barrier.dstAccessMask = {};
	stage.transfer.commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
												 barrier);
	stage.transfer.commandBuffer.end();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &stage.transfer.commandBuffer;
	g_info.queues.transfer.submit(submitInfo, stage.transfer.done);
	return stage.transfer.done;
}

Image vram::createImage(ImageInfo const& info)
{
	Image ret;
#if defined(LEVK_VKRESOURCE_NAMES)
	ASSERT(!info.name.empty(), "Unnamed buffer!");
	ret.name = info.name;
#endif
	vk::ImageCreateInfo imageInfo = info.createInfo;
	auto const queues = g_info.uniqueQueues(info.queueFlags);
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
	auto const requirements = g_info.device.getImageMemoryRequirements(ret.image);
	ret.queueFlags = info.queueFlags;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	ret.allocatedSize = requirements.size;
	g_allocations.at((size_t)ResourceType::eImage) += ret.allocatedSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		auto [size, unit] = utils::friendlySize(ret.allocatedSize);
#if defined(LEVK_VKRESOURCE_NAMES)
		LOG_I("== [{}] Image [{}] allocated: [{:.2f}{}] | {}", s_tName, ret.name, size, unit, logCount());
#else
		LOG_I("== [{}] Image allocated: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
	}
	return ret;
}

void vram::release(Buffer buffer, [[maybe_unused]] bool bSilent)
{
	if (buffer.buffer != vk::Buffer())
	{
		vmaDestroyBuffer(g_allocator, buffer.buffer, buffer.handle);
		g_allocations.at((size_t)ResourceType::eBuffer) -= buffer.writeSize;
		if constexpr (g_VRAM_bLogAllocs)
		{
			if (!bSilent)
			{
				if (buffer.info.actualSize)
				{
					auto [size, unit] = utils::friendlySize(buffer.writeSize);
#if defined(LEVK_VKRESOURCE_NAMES)
					LOG_I("-- [{}] Buffer [{}] released: [{:.2f}{}] | {}", s_tName, buffer.name, size, unit, logCount());
#else
					LOG_I("-- [{}] Buffer released: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
				}
			}
		}
	}
	return;
}

void vram::release(Image image)
{
	if (image.image != vk::Image())
	{
		vmaDestroyImage(g_allocator, image.image, image.handle);
		g_allocations.at((size_t)ResourceType::eImage) -= image.allocatedSize;
		if constexpr (g_VRAM_bLogAllocs)
		{
			if (image.info.actualSize > 0)
			{
				auto [size, unit] = utils::friendlySize(image.allocatedSize);
#if defined(LEVK_VKRESOURCE_NAMES)
				LOG_I("-- [{}] Image [{}] released: [{:.2f}{}] | {}", s_tName, image.name, size, unit, logCount());
#else
				LOG_I("-- [{}] Image released: [{:.2f}{}] | {}", s_tName, size, unit, logCount());
#endif
			}
		}
	}
	return;
}
} // namespace le::gfx
