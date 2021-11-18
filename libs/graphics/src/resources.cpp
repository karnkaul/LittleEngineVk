#include <core/utils/string.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/resources.hpp>

namespace le::graphics {
vk::SharingMode QShare::operator()(Device const& device, QFlags queues) const {
	return device.queues().familyIndices(queues).size() == 1 ? vk::SharingMode::eExclusive : desired;
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
	for (auto& count : m_allocations.arr) { count.store(0); }
	g_log.log(lvl::info, 1, "[{}] Memory constructed", g_name);
}

Memory::~Memory() {
	vmaDestroyAllocator(m_allocator);
	g_log.log(lvl::info, 1, "[{}] Memory destroyed", g_name);
}

void Memory::copy(vk::CommandBuffer cb, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cb.begin(beginInfo);
	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	cb.copyBuffer(src, dst, copyRegion);
	cb.end();
}

vk::ImageBlit imageBlit(Memory::ImgMeta const& src, Memory::ImgMeta const& dst, TPair<vk::Offset3D> srcOff, TPair<vk::Offset3D> dstOff) {
	vk::ImageBlit ret;
	ret.srcSubresource = vk::ImageSubresourceLayers(src.aspects, src.firstMip, src.firstLayer, src.layerCount);
	ret.dstSubresource = vk::ImageSubresourceLayers(dst.aspects, dst.firstMip, dst.firstLayer, dst.layerCount);
	vk::Offset3D offsets[] = {srcOff.first, srcOff.second};
	std::size_t idx = 0;
	for (auto& off : ret.srcOffsets) { off = offsets[idx++]; }
	offsets[0] = dstOff.first;
	offsets[1] = dstOff.second;
	idx = 0;
	for (auto& off : ret.dstOffsets) { off = offsets[idx++]; }
	return ret;
}

void Memory::blit(vk::CommandBuffer cb, TPair<vk::Image> images, TPair<vk::Extent3D> extents, LayoutPair layouts, vk::Filter filter,
				  TPair<vk::ImageAspectFlags> aspects) {
	ImgMeta msrc, mdst;
	msrc.aspects = aspects.first;
	mdst.aspects = aspects.second;
	vk::Offset3D const osrc((int)extents.first.width, (int)extents.first.height, (int)extents.first.depth);
	vk::Offset3D const odst((int)extents.second.width, (int)extents.second.height, (int)extents.second.depth);
	cb.blitImage(images.first, layouts.first, images.second, layouts.second, imageBlit(msrc, mdst, {{}, osrc}, {{}, odst}), filter);
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

vk::BufferImageCopy Memory::bufferImageCopy(vk::Extent3D extent, vk::ImageAspectFlags aspects, vk::DeviceSize offset, u32 layerIdx, u32 layerCount) {
	vk::BufferImageCopy ret;
	ret.bufferOffset = offset;
	ret.bufferRowLength = 0;
	ret.bufferImageHeight = 0;
	ret.imageSubresource.aspectMask = aspects;
	ret.imageSubresource.mipLevel = 0;
	ret.imageSubresource.baseArrayLayer = layerIdx;
	ret.imageSubresource.layerCount = layerCount;
	ret.imageOffset = vk::Offset3D(0, 0, 0);
	ret.imageExtent = extent;
	return ret;
}

void Memory::copy(vk::CommandBuffer cb, vk::Buffer src, vk::Image dst, vAP<vk::BufferImageCopy> regions, ImgMeta const& meta) {
	using vkstg = vk::PipelineStageFlagBits;
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cb.begin(beginInfo);
	ImgMeta first = meta, second = meta;
	first.layouts.second = vk::ImageLayout::eTransferDstOptimal;
	first.access.second = second.access.first = vk::AccessFlagBits::eTransferWrite;
	first.stages = {vkstg::eTopOfPipe | meta.stages.first, vkstg::eTransfer};
	second.layouts.first = vk::ImageLayout::eTransferDstOptimal;
	second.stages = {vkstg::eTransfer, vkstg::eBottomOfPipe | meta.stages.second};
	imageBarrier(cb, dst, first);
	cb.copyBufferToImage(src, dst, vk::ImageLayout::eTransferDstOptimal, regions);
	imageBarrier(cb, dst, second);
	cb.end();
}

Buffer::Buffer(not_null<Memory*> memory, CreateInfo const& info) : Resource(memory) {
	Device& device = *memory->m_device;
	vk::BufferCreateInfo bufferInfo;
	m_storage.writeSize = bufferInfo.size = info.size;
	bufferInfo.usage = info.usage;
	auto const indices = device.queues().familyIndices(info.queueFlags);
	bufferInfo.sharingMode = info.share(device, info.queueFlags);
	bufferInfo.queueFamilyIndexCount = (u32)indices.size();
	bufferInfo.pQueueFamilyIndices = indices.data();
	VmaAllocationCreateInfo createInfo = {};
	createInfo.usage = info.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
	VkBuffer vkBuffer;
	if (vmaCreateBuffer(memory->m_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &m_data.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	m_storage.buffer = vk::Buffer(vkBuffer);
	m_data.queueFlags = info.queueFlags;
	m_data.mode = bufferInfo.sharingMode;
	m_storage.usage = info.usage;
	m_storage.type = info.vmaUsage == VMA_MEMORY_USAGE_GPU_ONLY ? Buffer::Type::eGpuOnly : Buffer::Type::eCpuToGpu;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(memory->m_allocator, m_data.handle, &allocationInfo);
	m_data.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), allocationInfo.offset, allocationInfo.size};
	memory->m_allocations[kind_v].fetch_add(m_storage.writeSize);
}

void Buffer::exchg(Buffer& lhs, Buffer& rhs) noexcept {
	Resource::exchg(lhs, rhs);
	std::swap(lhs.m_storage, rhs.m_storage);
}

Buffer::~Buffer() {
	Memory& m = *m_memory;
	if (m_storage.pMap) { vmaUnmapMemory(m.m_allocator, m_data.handle); }
	if (!Device::default_v(m_storage.buffer)) {
		m.m_allocations[kind_v].fetch_sub(m_storage.writeSize);
		auto del = [a = m.m_allocator, b = m_storage.buffer, h = m_data.handle]() { vmaDestroyBuffer(a, static_cast<VkBuffer>(b), h); };
		m.m_device->defer(del);
	}
}

void const* Buffer::map() {
	if (m_storage.type != Buffer::Type::eCpuToGpu) {
		g_log.log(lvl::error, 1, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_storage.pMap && m_storage.writeSize > 0) { vmaMapMemory(m_memory->m_allocator, m_data.handle, &m_storage.pMap); }
	return mapped();
}

bool Buffer::unmap() {
	if (m_storage.pMap) {
		vmaUnmapMemory(m_memory->m_allocator, m_data.handle);
		m_storage.pMap = nullptr;
		return true;
	}
	return false;
}

bool Buffer::write(void const* pData, vk::DeviceSize size, vk::DeviceSize offset) {
	if (m_storage.type != Buffer::Type::eCpuToGpu) {
		g_log.log(lvl::error, 1, "[{}] Attempt to write to GPU-only Buffer!", g_name);
		return false;
	}
	if (!Device::default_v(m_data.alloc.memory) && !Device::default_v(m_storage.buffer)) {
		if (size == 0) { size = m_storage.writeSize - offset; }
		if (auto pMap = map()) {
			void* pStart = (void*)((char*)pMap + offset);
			std::memcpy(pStart, pData, (std::size_t)size);
			++m_storage.writeCount;
			return true;
		}
	}
	return false;
}

Image::Image(not_null<Memory*> memory, CreateInfo const& info) : Resource(memory) {
	Device& d = *memory->m_device;
	vk::ImageCreateInfo imageInfo = info.createInfo;
	auto const indices = d.queues().familyIndices(info.queueFlags);
	imageInfo.sharingMode = info.share(d, info.queueFlags);
	imageInfo.queueFamilyIndexCount = (u32)indices.size();
	imageInfo.pQueueFamilyIndices = indices.data();
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = info.vmaUsage;
	allocInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(info.preferred);
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (vmaCreateImage(memory->m_allocator, &vkImageInfo, &allocInfo, &vkImage, &m_data.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	m_storage.extent = info.createInfo.extent;
	m_storage.image = vk::Image(vkImage);
	m_storage.usage = info.createInfo.usage;
	m_storage.layout = info.createInfo.initialLayout;
	auto const requirements = d.device().getImageMemoryRequirements(m_storage.image);
	m_data.queueFlags = info.queueFlags;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(memory->m_allocator, m_data.handle, &allocationInfo);
	m_data.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), allocationInfo.offset, allocationInfo.size};
	m_storage.allocatedSize = requirements.size;
	m_data.mode = imageInfo.sharingMode;
	memory->m_allocations[kind_v].fetch_add(m_storage.allocatedSize);
	if (info.view.aspects != vk::ImageAspectFlags() && info.view.format != vk::Format()) {
		m_storage.view = d.makeImageView(m_storage.image, info.view.format, info.view.aspects, info.view.type);
	}
}

Image::~Image() {
	if (!Device::default_v(m_storage.image)) {
		Memory& m = *m_memory;
		m.m_allocations[kind_v].fetch_sub(m_storage.allocatedSize);
		auto del = [a = m.m_allocator, i = m_storage.image, h = m_data.handle, d = m.m_device, v = m_storage.view]() mutable {
			d->destroy(v);
			vmaDestroyImage(a, static_cast<VkImage>(i), h);
		};
		m.m_device->defer(del);
	}
}

void Image::exchg(Image& lhs, Image& rhs) noexcept {
	Resource::exchg(lhs, rhs);
	std::swap(lhs.m_storage, rhs.m_storage);
}
} // namespace le::graphics
