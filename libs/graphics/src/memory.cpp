#include <core/log_channel.hpp>
#include <core/utils/expect.hpp>
#include <core/utils/string.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/memory.hpp>
#include <graphics/render/framebuffer.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
namespace {
constexpr bool hostVisible(VmaMemoryUsage usage) noexcept {
	return usage == VMA_MEMORY_USAGE_CPU_TO_GPU || usage == VMA_MEMORY_USAGE_GPU_TO_CPU || usage == VMA_MEMORY_USAGE_CPU_ONLY ||
		   usage == VMA_MEMORY_USAGE_CPU_COPY;
}

constexpr bool canMip(BlitCaps bc, vk::ImageTiling tiling) noexcept {
	if (tiling == vk::ImageTiling::eLinear) {
		return bc.linear.all(BlitFlags(BlitFlag::eLinearFilter, BlitFlag::eSrc, BlitFlag::eDst));
	} else {
		return bc.optimal.all(BlitFlags(BlitFlag::eLinearFilter, BlitFlag::eSrc, BlitFlag::eDst));
	}
}

vk::SharingMode sharingMode(Queues const& queues, QCaps const caps) {
	if (caps.all(QCaps(QType::eCompute, QType::eGraphics)) && queues.compute() != &queues.graphics()) { return vk::SharingMode::eConcurrent; }
	return vk::SharingMode::eExclusive;
}
} // namespace

Memory::Memory(not_null<Device*> device) : m_device(device) {
	VmaAllocatorCreateInfo allocatorInfo = {};
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	allocatorInfo.instance = static_cast<VkInstance>(device->instance());
	allocatorInfo.device = static_cast<VkDevice>(device->device());
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(device->physicalDevice().device);
	VmaVulkanFunctions vkFunc = {};
	vkFunc.vkGetPhysicalDeviceProperties = dl.vkGetPhysicalDeviceProperties;
	vkFunc.vkGetPhysicalDeviceMemoryProperties = dl.vkGetPhysicalDeviceMemoryProperties;
	vkFunc.vkAllocateMemory = dl.vkAllocateMemory;
	vkFunc.vkFreeMemory = dl.vkFreeMemory;
	vkFunc.vkMapMemory = dl.vkMapMemory;
	vkFunc.vkUnmapMemory = dl.vkUnmapMemory;
	vkFunc.vkFlushMappedMemoryRanges = dl.vkFlushMappedMemoryRanges;
	vkFunc.vkInvalidateMappedMemoryRanges = dl.vkInvalidateMappedMemoryRanges;
	vkFunc.vkBindBufferMemory = dl.vkBindBufferMemory;
	vkFunc.vkBindImageMemory = dl.vkBindImageMemory;
	vkFunc.vkGetBufferMemoryRequirements = dl.vkGetBufferMemoryRequirements;
	vkFunc.vkGetImageMemoryRequirements = dl.vkGetImageMemoryRequirements;
	vkFunc.vkCreateBuffer = dl.vkCreateBuffer;
	vkFunc.vkDestroyBuffer = dl.vkDestroyBuffer;
	vkFunc.vkCreateImage = dl.vkCreateImage;
	vkFunc.vkDestroyImage = dl.vkDestroyImage;
	vkFunc.vkCmdCopyBuffer = dl.vkCmdCopyBuffer;
	vkFunc.vkGetBufferMemoryRequirements2KHR = dl.vkGetBufferMemoryRequirements2KHR;
	vkFunc.vkGetImageMemoryRequirements2KHR = dl.vkGetImageMemoryRequirements2KHR;
	vkFunc.vkBindBufferMemory2KHR = dl.vkBindBufferMemory2KHR;
	vkFunc.vkBindImageMemory2KHR = dl.vkBindImageMemory2KHR;
	vkFunc.vkGetPhysicalDeviceMemoryProperties2KHR = dl.vkGetPhysicalDeviceMemoryProperties2KHR;
	allocatorInfo.pVulkanFunctions = &vkFunc;
	vmaCreateAllocator(&allocatorInfo, &m_allocator);
	for (auto& count : m_allocations.arr) { count.store(0); }
	logI(LC_LibUser, "[{}] Memory constructed", g_name);
}

Memory::~Memory() {
	vmaDestroyAllocator(m_allocator);
	logI(LC_LibUser, "[{}] Memory destroyed", g_name);
}

void Memory::copy(vk::CommandBuffer cb, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	cb.copyBuffer(src, dst, copyRegion);
}

void Memory::copy(vk::CommandBuffer cb, vk::Buffer src, vk::Image dst, vAP<vk::BufferImageCopy> regions, ImgMeta const& meta) {
	using vkstg = vk::PipelineStageFlagBits;
	ImgMeta first = meta, second = meta;
	first.layouts.second = second.layouts.first = vk::ImageLayout::eTransferDstOptimal;
	first.access.second = second.access.first = vk::AccessFlagBits::eTransferWrite;
	first.stages = {vkstg::eTopOfPipe | meta.stages.first, vkstg::eTransfer};
	second.stages = {vkstg::eTransfer, vkstg::eBottomOfPipe | meta.stages.second};
	imageBarrier(cb, dst, first);
	cb.copyBufferToImage(src, dst, vk::ImageLayout::eTransferDstOptimal, regions);
	imageBarrier(cb, dst, second);
}

void Memory::copy(vk::CommandBuffer cb, TPair<vk::Image> images, vk::Extent3D extent, vk::ImageAspectFlags aspects) {
	vk::ImageCopy region;
	region.extent = extent;
	vk::ImageSubresourceLayers subResource;
	subResource.aspectMask = aspects;
	subResource.baseArrayLayer = 0;
	subResource.layerCount = 1U;
	subResource.mipLevel = 0;
	region.srcSubresource = region.dstSubresource = subResource;
	cb.copyImage(images.first, vIL::eTransferSrcOptimal, images.second, vIL::eTransferDstOptimal, region);
}

void Memory::blit(vk::CommandBuffer cb, TPair<vk::Image> images, TPair<vk::Extent3D> extents, TPair<vk::ImageAspectFlags> aspects, BlitFilter filter) {
	ImgMeta msrc, mdst;
	msrc.aspects = aspects.first;
	mdst.aspects = aspects.second;
	vk::Offset3D const osrc((int)extents.first.width, (int)extents.first.height, (int)extents.first.depth);
	vk::Offset3D const odst((int)extents.second.width, (int)extents.second.height, (int)extents.second.depth);
	auto const vkFilter = filter == BlitFilter::eNearest ? vk::Filter::eNearest : vk::Filter::eLinear;
	cb.blitImage(images.first, vIL::eTransferSrcOptimal, images.second, vIL::eTransferDstOptimal, imageBlit({msrc, mdst}, {{}, osrc}, {{}, odst}), vkFilter);
}

void Memory::imageBarrier(vk::CommandBuffer cb, vk::Image image, ImgMeta const& meta) {
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = meta.layouts.first;
	barrier.newLayout = meta.layouts.second;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = meta.aspects;
	barrier.subresourceRange.baseMipLevel = meta.firstMip;
	barrier.subresourceRange.levelCount = meta.mipLevels;
	barrier.subresourceRange.baseArrayLayer = meta.firstLayer;
	barrier.subresourceRange.layerCount = meta.layerCount;
	barrier.srcAccessMask = meta.access.first;
	barrier.dstAccessMask = meta.access.second;
	cb.pipelineBarrier(meta.stages.first, meta.stages.second, {}, {}, {}, barrier);
}

vk::BufferImageCopy Memory::bufferImageCopy(vk::Extent3D extent, vk::ImageAspectFlags aspects, vk::DeviceSize offset, u32 layerIdx) {
	vk::BufferImageCopy ret;
	ret.bufferOffset = offset;
	ret.bufferRowLength = 0;
	ret.bufferImageHeight = 0;
	ret.imageSubresource.aspectMask = aspects;
	ret.imageSubresource.mipLevel = 0U;
	ret.imageSubresource.baseArrayLayer = layerIdx;
	ret.imageSubresource.layerCount = 1U;
	ret.imageOffset = vk::Offset3D(0, 0, 0);
	ret.imageExtent = extent;
	return ret;
}

vk::ImageBlit Memory::imageBlit(TPair<Memory::ImgMeta> const& meta, TPair<vk::Offset3D> const& srcOff, TPair<vk::Offset3D> const& dstOff) noexcept {
	vk::ImageBlit ret;
	ret.srcSubresource = vk::ImageSubresourceLayers(meta.first.aspects, meta.first.firstMip, meta.first.firstLayer, meta.first.layerCount);
	ret.dstSubresource = vk::ImageSubresourceLayers(meta.second.aspects, meta.second.firstMip, meta.second.firstLayer, meta.second.layerCount);
	vk::Offset3D offsets[] = {srcOff.first, srcOff.second};
	std::size_t idx = 0;
	for (auto& off : ret.srcOffsets) { off = offsets[idx++]; }
	offsets[0] = dstOff.first;
	offsets[1] = dstOff.second;
	idx = 0;
	for (auto& off : ret.dstOffsets) { off = offsets[idx++]; }
	return ret;
}

Buffer::Buffer(not_null<Memory*> memory, CreateInfo const& info) : m_memory(memory) {
	Device& device = *memory->m_device;
	vk::BufferCreateInfo bufferInfo;
	m_storage.allocation.size = bufferInfo.size = info.size;
	bufferInfo.usage = info.usage;
	bufferInfo.sharingMode = sharingMode(device.queues(), info.qcaps);
	bufferInfo.queueFamilyIndexCount = 1U;
	u32 const family = device.queues().primary().family();
	bufferInfo.pQueueFamilyIndices = &family;
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
	VkBuffer vkBuffer;
	if (vmaCreateBuffer(memory->m_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &m_storage.allocation.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	m_storage.buffer = vk::Buffer(vkBuffer);
	m_storage.allocation.qcaps = info.qcaps;
	m_storage.allocation.mode = bufferInfo.sharingMode;
	m_storage.usage = info.usage;
	m_storage.type = info.vmaUsage == VMA_MEMORY_USAGE_GPU_ONLY ? Buffer::Type::eGpuOnly : Buffer::Type::eCpuToGpu;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(memory->m_allocator, m_storage.allocation.handle, &allocationInfo);
	m_storage.allocation.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), vk::DeviceSize(allocationInfo.offset), vk::DeviceSize(allocationInfo.size)};
	memory->m_allocations[allocation_type_v].fetch_add(m_storage.allocation.size);
}

void Buffer::exchg(Buffer& lhs, Buffer& rhs) noexcept {
	std::swap(lhs.m_storage, rhs.m_storage);
	std::swap(lhs.m_memory, rhs.m_memory);
}

Buffer::~Buffer() {
	Memory& m = *m_memory;
	if (m_storage.allocation.data) { vmaUnmapMemory(m.m_allocator, m_storage.allocation.handle); }
	if (!Device::default_v(m_storage.buffer)) {
		m.m_allocations[allocation_type_v].fetch_sub(m_storage.allocation.size);
		auto del = [a = m.m_allocator, b = m_storage.buffer, h = m_storage.allocation.handle]() { vmaDestroyBuffer(a, static_cast<VkBuffer>(b), h); };
		m.m_device->defer(del);
	}
}

void const* Buffer::map() {
	if (m_storage.type != Buffer::Type::eCpuToGpu) {
		logE(LC_LibUser, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_storage.allocation.data && m_storage.allocation.size > 0) {
		vmaMapMemory(m_memory->m_allocator, m_storage.allocation.handle, &m_storage.allocation.data);
	}
	return mapped();
}

bool Buffer::unmap() {
	if (m_storage.allocation.data) {
		vmaUnmapMemory(m_memory->m_allocator, m_storage.allocation.handle);
		m_storage.allocation.data = nullptr;
		return true;
	}
	return false;
}

bool Buffer::write(void const* data, vk::DeviceSize size, vk::DeviceSize offset) {
	if (m_storage.type != Buffer::Type::eCpuToGpu) {
		logE(LC_LibUser, "[{}] Attempt to write to GPU-only Buffer!", g_name);
		return false;
	}
	EXPECT(size + offset <= m_storage.allocation.size);
	if (size + offset > m_storage.allocation.size) { return false; }
	if (!Device::default_v(m_storage.allocation.alloc.memory) && !Device::default_v(m_storage.buffer)) {
		if (size == 0) { size = m_storage.allocation.size - offset; }
		if (auto ptr = map()) {
			char* start = (char*)ptr + offset;
			std::memcpy(start, data, (std::size_t)size);
			++m_storage.writeCount;
			return true;
		}
	}
	return false;
}

Image::CreateInfo Image::info(Extent2D extent, vk::ImageUsageFlags usage, vk::ImageAspectFlags view, VmaMemoryUsage vmaUsage, vk::Format format) noexcept {
	CreateInfo ret;
	ret.createInfo.extent = vk::Extent3D(extent.x, extent.y, 1);
	ret.createInfo.usage = usage;
	ret.vmaUsage = vmaUsage;
	bool const linear = vmaUsage != VMA_MEMORY_USAGE_UNKNOWN && vmaUsage != VMA_MEMORY_USAGE_GPU_ONLY && vmaUsage != VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
	ret.view.aspects = view;
	ret.qcaps = QType::eGraphics;
	if (view != vk::ImageAspectFlags()) {
		ret.view.format = format;
		ret.view.type = vk::ImageViewType::e2D;
	}
	ret.createInfo.format = format;
	ret.createInfo.tiling = linear ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
	ret.createInfo.imageType = vk::ImageType::e2D;
	ret.createInfo.mipLevels = 1U;
	ret.createInfo.arrayLayers = 1U;
	return ret;
}

Image::CreateInfo Image::textureInfo(Extent2D extent, vk::Format format, bool mips) noexcept {
	auto ret = info(extent, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format);
	ret.mipMaps = mips;
	return ret;
}

Image::CreateInfo Image::cubemapInfo(Extent2D extent, vk::Format format) noexcept {
	auto ret = info(extent, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format);
	ret.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	ret.createInfo.arrayLayers = 6U;
	ret.view.type = vk::ImageViewType::eCube;
	return ret;
}

Image::CreateInfo Image::storageInfo(Extent2D extent, vk::Format format) noexcept {
	auto ret = info(extent, vIUFB::eTransferDst, vk::ImageAspectFlags(), VMA_MEMORY_USAGE_GPU_TO_CPU, format);
	ret.createInfo.tiling = vk::ImageTiling::eLinear;
	return ret;
}

u32 Image::mipLevels(Extent2D extent) noexcept { return static_cast<u32>(std::floor(std::log2(std::max(extent.x, extent.y)))) + 1U; }

ImageRef Image::ref() const noexcept {
	bool const linear = m_storage.tiling == vk::ImageTiling::eLinear;
	return ImageRef{ImageView{m_storage.image, m_storage.view, extent2D(), m_storage.format}, linear};
}

Image::Image(not_null<Memory*> memory, CreateInfo const& info) : m_memory(memory) {
	Device& d = *memory->m_device;
	u32 const family = d.queues().primary().family();
	vk::ImageCreateInfo imageInfo = info.createInfo;
	imageInfo.sharingMode = sharingMode(d.queues(), info.qcaps);
	imageInfo.queueFamilyIndexCount = 1U;
	imageInfo.pQueueFamilyIndices = &family;
	auto const blitCaps = memory->m_device->physicalDevice().blitCaps(imageInfo.format);
	imageInfo.mipLevels = info.mipMaps && canMip(blitCaps, imageInfo.tiling) ? mipLevels(cast(imageInfo.extent)) : 1U;
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = info.vmaUsage;
	allocInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(info.preferred);
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (auto res = vmaCreateImage(memory->m_allocator, &vkImageInfo, &allocInfo, &vkImage, &m_storage.allocation.handle, nullptr); res != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	m_storage.extent = imageInfo.extent;
	m_storage.image = vk::Image(vkImage);
	m_storage.usage = imageInfo.usage;
	m_storage.format = imageInfo.format;
	m_storage.vmaUsage = info.vmaUsage;
	m_storage.mipCount = imageInfo.mipLevels;
	m_storage.tiling = imageInfo.tiling;
	m_storage.blitFlags = imageInfo.tiling == vk::ImageTiling::eLinear ? blitCaps.linear : blitCaps.optimal;
	auto const requirements = d.device().getImageMemoryRequirements(m_storage.image);
	m_storage.allocation.qcaps = info.qcaps;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(memory->m_allocator, m_storage.allocation.handle, &allocationInfo);
	m_storage.allocation.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), vk::DeviceSize(allocationInfo.offset), vk::DeviceSize(allocationInfo.size)};
	m_storage.allocation.size = requirements.size;
	m_storage.allocation.mode = imageInfo.sharingMode;
	memory->m_allocations[allocation_type_v].fetch_add(m_storage.allocation.size);
	if (info.view.aspects != vk::ImageAspectFlags() && info.view.format != vk::Format()) {
		m_storage.view = d.makeImageView(m_storage.image, info.view.format, info.view.aspects, info.view.type, imageInfo.mipLevels);
		m_storage.viewType = info.view.type;
	}
}

Image::~Image() {
	Memory& m = *m_memory;
	if (m_storage.allocation.data) { vmaUnmapMemory(m.m_allocator, m_storage.allocation.handle); }
	if (!Device::default_v(m_storage.image) && m_storage.allocation.size > 0U) {
		m.m_allocations[allocation_type_v].fetch_sub(m_storage.allocation.size);
		auto del = [a = m.m_allocator, i = m_storage.image, h = m_storage.allocation.handle, d = m.m_device, v = m_storage.view]() mutable {
			d->destroy(v);
			vmaDestroyImage(a, static_cast<VkImage>(i), h);
		};
		m.m_device->defer(del);
	}
}

void const* Image::map() {
	if (!hostVisible(m_storage.vmaUsage)) {
		logE(LC_LibUser, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_storage.allocation.data) { vmaMapMemory(m_memory->m_allocator, m_storage.allocation.handle, &m_storage.allocation.data); }
	return mapped();
}

bool Image::unmap() {
	if (m_storage.allocation.data) {
		vmaUnmapMemory(m_memory->m_allocator, m_storage.allocation.handle);
		m_storage.allocation.data = nullptr;
		return true;
	}
	return false;
}

void Image::exchg(Image& lhs, Image& rhs) noexcept {
	std::swap(lhs.m_storage, rhs.m_storage);
	std::swap(lhs.m_memory, rhs.m_memory);
}
} // namespace le::graphics
