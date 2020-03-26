#include <array>
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
	eBuffer = 0,
	eImage,
	eCOUNT_
};

static std::string const s_tName = utils::tName<VRAM>();
std::array<u64, (size_t)ResourceType::eCOUNT_> g_allocations;

Buffer g_staging;
vk::CommandPool g_stagingPool;

Buffer createStagingBuffer(vk::DeviceSize size)
{
	gfx::BufferInfo info;
	info.size = size;
	info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	info.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
	info.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
	return vram::createBuffer(info);
}

TResult<TransferOp> copyBuffer(Buffer const& src, Buffer const& dst, vk::CommandPool commandPool, vk::DeviceSize size)
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
	TransferOp ret;
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	ret.commandBuffer = g_info.device.allocateCommandBuffers(allocInfo).front();
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
	ret.transferred = g_info.device.createFence({});
	g_info.queues.transfer.submit(submitInfo, ret.transferred);
	return ret;
}

[[maybe_unused]] std::string logCount()
{
	auto [bufferSize, bufferUnit] = utils::friendlySize(g_allocations.at((size_t)ResourceType::eBuffer));
	auto const [imageSize, imageUnit] = utils::friendlySize(g_allocations.at((size_t)ResourceType::eImage));
	return fmt::format("Buffers: [{}{}]; Images: [{}{}]", bufferSize, bufferUnit, imageSize, imageUnit);
}
} // namespace

void vram::init(vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice)
{
	if (g_allocator == VmaAllocator())
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.instance = instance;
		allocatorInfo.device = device;
		allocatorInfo.physicalDevice = physicalDevice;
		vmaCreateAllocator(&allocatorInfo, &g_allocator);
		for (auto& val : g_allocations)
		{
			val = 0;
		}
		LOG_I("[{}] initialised", s_tName);
	}
	return;
}

void vram::deinit()
{
	if (g_staging.buffer != vk::Buffer())
	{
		release(g_staging);
		g_staging = Buffer();
	}
	vkDestroy(g_stagingPool);
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

Buffer vram::createBuffer(BufferInfo const& info)
{
	Buffer ret;
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
		auto [size, unit] = utils::friendlySize(ret.writeSize);
		LOG_I("== [{}] Buffer allocated: [{}{}] | {}", s_tName, size, unit, logCount());
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
		auto pMem = g_info.device.mapMemory(buffer.info.memory, buffer.info.offset, size);
		std::memcpy(pMem, pData, (size_t)buffer.writeSize);
		g_info.device.unmapMemory(buffer.info.memory);
		return true;
	}
	return false;
}

TResult<TransferOp> vram::copy(Buffer const& src, Buffer const& dst, vk::DeviceSize size)
{
	return copyBuffer(src, dst, g_stagingPool, size);
}

bool vram::stage(Buffer const& deviceBuffer, void const* pData, vk::DeviceSize size)
{
	if (size == 0)
	{
		size = deviceBuffer.writeSize;
	}
	if (g_staging.writeSize < size)
	{
		auto const size = g_staging.writeSize == 0 ? deviceBuffer.writeSize : std::max(g_staging.writeSize * 2, deviceBuffer.writeSize);
		release(g_staging);
		g_staging = createStagingBuffer(size);
	}
	if (g_stagingPool == vk::CommandPool())
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.queueFamilyIndex = g_info.queueFamilyIndices.transfer;
		g_stagingPool = g_info.device.createCommandPool(commandPoolCreateInfo);
	}
	write(g_staging, pData, size);
	auto [transferOp, bResult] = copyBuffer(g_staging, deviceBuffer, g_stagingPool, size);
	if (bResult)
	{
		wait(transferOp.transferred);
		g_info.device.freeCommandBuffers(g_stagingPool, transferOp.commandBuffer);
		vkDestroy(transferOp.transferred);
		return true;
	}
	return false;
}

Image vram::createImage(ImageInfo const& info)
{
	Image ret;
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = info.type;
	imageInfo.extent = info.size;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = info.format;
	imageInfo.tiling = info.tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = info.usage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	auto const queues = g_info.uniqueQueues(info.queueFlags);
	imageInfo.sharingMode = queues.mode;
	imageInfo.queueFamilyIndexCount = (u32)queues.indices.size();
	imageInfo.pQueueFamilyIndices = queues.indices.data();
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (vmaCreateImage(g_allocator, &vkImageInfo, &createInfo, &vkImage, &ret.handle, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Allocation error");
	}
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
		LOG_I("== [{}] Image allocated: [{}{}] | {}", s_tName, size, unit, logCount());
	}
	return ret;
}

void vram::release(Buffer buffer)
{
	vmaDestroyBuffer(g_allocator, buffer.buffer, buffer.handle);
	g_allocations.at((size_t)ResourceType::eBuffer) -= buffer.writeSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		if (buffer.info.actualSize)
		{
			auto [size, unit] = utils::friendlySize(buffer.writeSize);
			LOG_I("-- [{}] Buffer released: [{}{}] | {}", s_tName, size, unit, logCount());
		}
	}
	return;
}

void vram::release(Image image)
{
	vmaDestroyImage(g_allocator, image.image, image.handle);
	g_allocations.at((size_t)ResourceType::eImage) -= image.allocatedSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		if (image.info.actualSize > 0)
		{
			auto [size, unit] = utils::friendlySize(image.allocatedSize);
			LOG_I("-- [{}] Image released: [{}{}] | {}", s_tName, size, unit, logCount());
		}
	}
	return;
}
} // namespace le::gfx
