#include <ktl/enumerate.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/device/device.hpp>
#include <levk/graphics/memory.hpp>

namespace le::graphics {
vk::SharingMode Memory::sharingMode(Queues const& queues, QCaps const caps) {
	if (caps.all(QCaps(QType::eCompute, QType::eGraphics)) && queues.compute() != &queues.graphics()) { return vk::SharingMode::eConcurrent; }
	return vk::SharingMode::eExclusive;
}

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
	logI(LC_LibUser, "[{}] Memory constructed", g_name);
}

Memory::~Memory() {
	vmaDestroyAllocator(m_allocator);
	logI(LC_LibUser, "[{}] Memory destroyed", g_name);
}

void Memory::clear(vk::CommandBuffer cb, ImageRef const& image, LayerMip const& layerMip, vk::ImageLayout layout, Colour colour) {
	vk::ImageSubresourceRange const isr(vIAFB::eColor, layerMip.mip.first, layerMip.mip.count, layerMip.layer.first, layerMip.layer.count);
	auto const c = colour.toVec4();
	vk::ClearColorValue const clear(std::array<float, 4>{{c.x, c.y, c.z, c.w}});
	cb.clearColorImage(image.image, layout, clear, isr);
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
	if (second.layouts.second != vIL::eUndefined) { imageBarrier(cb, dst, second); }
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
	barrier.subresourceRange.baseMipLevel = meta.layerMip.mip.first;
	barrier.subresourceRange.levelCount = meta.layerMip.mip.count;
	barrier.subresourceRange.baseArrayLayer = meta.layerMip.layer.first;
	barrier.subresourceRange.layerCount = meta.layerMip.layer.count;
	barrier.srcAccessMask = meta.access.first;
	barrier.dstAccessMask = meta.access.second;
	cb.pipelineBarrier(meta.stages.first, meta.stages.second, {}, {}, {}, barrier);
}

vk::BufferImageCopy Memory::bufferImageCopy(vk::Extent3D extent, vk::ImageAspectFlags aspects, vk::DeviceSize bo, glm::ivec2 io, u32 layerIdx) {
	vk::BufferImageCopy ret;
	ret.bufferOffset = bo;
	ret.bufferRowLength = 0;
	ret.bufferImageHeight = 0;
	ret.imageSubresource.aspectMask = aspects;
	ret.imageSubresource.mipLevel = 0U;
	ret.imageSubresource.baseArrayLayer = layerIdx;
	ret.imageSubresource.layerCount = 1U;
	ret.imageOffset = vk::Offset3D(io.x, io.y, 0);
	ret.imageExtent = extent;
	return ret;
}

vk::ImageBlit Memory::imageBlit(TPair<Memory::ImgMeta> const& meta, TPair<vk::Offset3D> const& srcOff, TPair<vk::Offset3D> const& dstOff) noexcept {
	vk::ImageBlit ret;
	auto const& flm = meta.first.layerMip;
	auto const& slm = meta.second.layerMip;
	ret.srcSubresource = vk::ImageSubresourceLayers(meta.first.aspects, flm.mip.first, flm.layer.first, flm.layer.count);
	ret.dstSubresource = vk::ImageSubresourceLayers(meta.second.aspects, slm.mip.first, slm.layer.first, slm.layer.count);
	vk::Offset3D offsets[] = {srcOff.first, srcOff.second};
	for (auto [off, idx] : ktl::enumerate(ret.srcOffsets)) { off = offsets[idx]; }
	offsets[0] = dstOff.first;
	offsets[1] = dstOff.second;
	for (auto [off, idx] : ktl::enumerate(ret.dstOffsets)) { off = offsets[idx]; }
	return ret;
}

std::optional<Memory::Resource> Memory::makeBuffer(AllocInfo const& ai, vk::BufferCreateInfo const& bci) const {
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = ai.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bci);
	VkBuffer vkBuffer;
	VmaAllocation handle;
	if (vmaCreateBuffer(m_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &handle, nullptr) != VK_SUCCESS) { return std::nullopt; }
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(m_allocator, handle, &allocationInfo);
	Resource ret;
	ret.resource = vk::Buffer(vkBuffer);
	ret.handle = handle;
	ret.allocator = m_allocator;
	ret.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), vk::DeviceSize(allocationInfo.offset), vk::DeviceSize(allocationInfo.size)};
	ret.size = bci.size;
	ret.qcaps = ai.qcaps;
	ret.mode = bci.sharingMode;
	return ret;
}

std::optional<Memory::Resource> Memory::makeImage(AllocInfo const& ai, vk::ImageCreateInfo const& ici) const {
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = ai.vmaUsage;
	allocInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(ai.preferred);
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(ici);
	VkImage vkImage;
	VmaAllocation handle;
	if (auto res = vmaCreateImage(m_allocator, &vkImageInfo, &allocInfo, &vkImage, &handle, nullptr); res != VK_SUCCESS) { return std::nullopt; }
	auto image = vk::Image(vkImage);
	auto const requirements = m_device->device().getImageMemoryRequirements(image);
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(m_allocator, handle, &allocationInfo);
	Resource ret;
	ret.resource = image;
	ret.handle = handle;
	ret.allocator = m_allocator;
	ret.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), vk::DeviceSize(allocationInfo.offset), vk::DeviceSize(allocationInfo.size)};
	ret.size = requirements.size;
	ret.mode = ici.sharingMode;
	ret.qcaps = ai.qcaps;
	m_device->m_layouts.force(image, ici.initialLayout);
	return ret;
}

void* Memory::map(Resource& out_resource) const {
	if (!out_resource.data) { vmaMapMemory(m_allocator, out_resource.handle, &out_resource.data); }
	return out_resource.data;
}

void Memory::unmap(Resource& out_resource) const {
	if (out_resource.data) {
		vmaUnmapMemory(m_allocator, out_resource.handle);
		out_resource.data = {};
	}
}

void Memory::Deleter::operator()(Device& device, Resource const& resource) const {
	if (resource.data) { vmaUnmapMemory(resource.allocator, resource.handle); }
	resource.resource.visit(ktl::koverloaded{
		[&](vk::Buffer buffer) { vmaDestroyBuffer(resource.allocator, static_cast<VkBuffer>(buffer), resource.handle); },
		[&](vk::Image image) {
			vmaDestroyImage(resource.allocator, static_cast<VkImage>(image), resource.handle);
			device.m_layouts.force(image, vk::ImageLayout::eUndefined);
		},
	});
}
} // namespace le::graphics
