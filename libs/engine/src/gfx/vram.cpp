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
	auto const flags = QFlags({QFlag::eGraphics, QFlag::eTransfer});
	bufferInfo.sharingMode = gfx::g_info.sharingMode(flags);
	auto const queueIndices = gfx::g_info.uniqueQueueIndices(flags);
	bufferInfo.queueFamilyIndexCount = (u32)queueIndices.size();
	bufferInfo.pQueueFamilyIndices = queueIndices.data();
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
	VkBuffer vkBuffer;
	if (vmaCreateBuffer(g_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &ret.handle, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Allocation error");
	}
	ret.buffer = vkBuffer;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	g_allocations.at((size_t)ResourceType::eBuffer) += ret.info.actualSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		auto [size, unit] = utils::friendlySize(ret.info.actualSize);
		LOG_I("== [{}] Buffer allocated: [{}{}] | {}", s_tName, size, unit, logCount());
	}
	return ret;
}

void vram::copy(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, TransferOp* pOp)
{
	ASSERT(pOp, "Null pointer!");
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = pOp->pool;
	allocInfo.commandBufferCount = 1;
	auto commandBuffer = g_info.device.allocateCommandBuffers(allocInfo).front();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);
	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	commandBuffer.copyBuffer(src, dst, copyRegion);
	commandBuffer.end();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	pOp->transferred = g_info.device.createFence({});
	pOp->queue.submit(submitInfo, pOp->transferred);
	return;
}

bool vram::write(Buffer buffer, void const* pData)
{
	if (buffer.info.memory != vk::DeviceMemory() && buffer.buffer != vk::Buffer())
	{
		auto pMem = g_info.device.mapMemory(buffer.info.memory, buffer.info.offset, buffer.writeSize);
		std::memcpy(pMem, pData, (size_t)buffer.writeSize);
		g_info.device.unmapMemory(buffer.info.memory);
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
	auto const queueIndices = g_info.uniqueQueueIndices(info.queueFlags);
	imageInfo.queueFamilyIndexCount = (u32)queueIndices.size();
	imageInfo.pQueueFamilyIndices = queueIndices.data();
	imageInfo.sharingMode = g_info.sharingMode(info.queueFlags);
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (vmaCreateImage(g_allocator, &vkImageInfo, &createInfo, &vkImage, &ret.handle, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Allocation error");
	}
	ret.image = vkImage;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(g_allocator, ret.handle, &allocationInfo);
	ret.info = {allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size};
	g_allocations.at((size_t)ResourceType::eImage) += ret.info.actualSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		auto [size, unit] = utils::friendlySize(ret.info.actualSize);
		LOG_I("== [{}] Image allocated: [{}{}] | {}", s_tName, size, unit, logCount());
	}
	return ret;
}

void vram::release(Buffer buffer)
{
	vmaDestroyBuffer(g_allocator, buffer.buffer, buffer.handle);
	g_allocations.at((size_t)ResourceType::eBuffer) -= buffer.info.actualSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		auto [size, unit] = utils::friendlySize(buffer.info.actualSize);
		LOG_I("-- [{}] Buffer released: [{}{}] | {}", s_tName, size, unit, logCount());
	}
	return;
}

void vram::release(Image image)
{
	vmaDestroyImage(g_allocator, image.image, image.handle);
	g_allocations.at((size_t)ResourceType::eImage) -= image.info.actualSize;
	if constexpr (g_VRAM_bLogAllocs)
	{
		auto [size, unit] = utils::friendlySize(image.info.actualSize);
		LOG_I("-- [{}] Image released: [{}{}] | {}", s_tName, size, unit, logCount());
	}
	return;
}
} // namespace le::gfx
