#include <core/utils/string.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/resources.hpp>

namespace le::graphics {
vk::SharingMode QShare::operator()(Device const& device, QFlags queues) const {
	return device.queues().familyIndices(queues).size() == 1 ? vk::SharingMode::eExclusive : desired;
}

Memory::Memory(Device& device) : m_device(device) {
	VmaAllocatorCreateInfo allocatorInfo = {};
	Instance& inst = device.m_instance;
	auto dl = inst.loader();
	allocatorInfo.instance = static_cast<VkInstance>(inst.instance());
	allocatorInfo.device = static_cast<VkDevice>(device.device());
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(device.physicalDevice().device);
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
	for (auto& count : m_allocations) {
		count.store(0);
	}
	g_log.log(lvl::info, 1, "[{}] Memory constructed", g_name);
}

Memory::~Memory() {
	vmaDestroyAllocator(m_allocator);
	g_log.log(lvl::info, 1, "[{}] Memory destroyed", g_name);
}

Buffer::Buffer(Memory& memory, CreateInfo const& info) : Resource(memory) {
	Device& device = memory.m_device;
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
	if (vmaCreateBuffer(memory.m_allocator, &vkBufferInfo, &createInfo, &vkBuffer, &m_data.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	m_storage.buffer = vk::Buffer(vkBuffer);
	m_data.queueFlags = info.queueFlags;
	m_data.mode = bufferInfo.sharingMode;
	m_storage.usage = info.usage;
	m_storage.type = info.vmaUsage == VMA_MEMORY_USAGE_GPU_ONLY ? Buffer::Type::eGpuOnly : Buffer::Type::eCpuToGpu;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(memory.m_allocator, m_data.handle, &allocationInfo);
	m_data.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), allocationInfo.offset, allocationInfo.size};
	memory.m_allocations[(std::size_t)type].fetch_add(m_storage.writeSize);
}

Buffer::Buffer(Buffer&& rhs) : Resource(rhs.m_memory), m_storage(std::exchange(rhs.m_storage, Storage())) {
	m_data = std::exchange(rhs.m_data, Data());
}

Buffer& Buffer::operator=(Buffer&& rhs) {
	if (&rhs != this) {
		destroy();
		m_data = std::exchange(rhs.m_data, Data());
		m_storage = std::exchange(rhs.m_storage, Storage());
		m_memory = rhs.m_memory;
	}
	return *this;
}

Buffer::~Buffer() {
	destroy();
}

void Buffer::destroy() {
	Memory& m = m_memory;
	if (m_storage.pMap) {
		vmaUnmapMemory(m.m_allocator, m_data.handle);
	}
	if (!Device::default_v(m_storage.buffer)) {
		m.m_allocations[(std::size_t)type].fetch_sub(m_storage.writeSize);
		auto del = [a = m.m_allocator, b = m_storage.buffer, h = m_data.handle]() { vmaDestroyBuffer(a, static_cast<VkBuffer>(b), h); };
		m.m_device.get().defer(del);
	}
}

void const* Buffer::map() {
	if (m_storage.type != Buffer::Type::eCpuToGpu) {
		g_log.log(lvl::error, 1, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_storage.pMap && m_storage.writeSize > 0) {
		vmaMapMemory(m_memory.get().m_allocator, m_data.handle, &m_storage.pMap);
	}
	return mapped();
}

bool Buffer::unmap() {
	if (m_storage.pMap) {
		vmaUnmapMemory(m_memory.get().m_allocator, m_data.handle);
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
		if (size == 0) {
			size = m_storage.writeSize - offset;
		}
		if (auto pMap = map()) {
			void* pStart = (void*)((char*)pMap + offset);
			std::memcpy(pStart, pData, (std::size_t)size);
			++m_storage.writeCount;
			return true;
		}
	}
	return false;
}

Image::Image(Memory& memory, CreateInfo const& info) : Resource(memory) {
	Device& d = memory.m_device;
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
	if (vmaCreateImage(memory.m_allocator, &vkImageInfo, &allocInfo, &vkImage, &m_data.handle, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Allocation error");
	}
	m_storage.extent = info.createInfo.extent;
	m_storage.image = vk::Image(vkImage);
	auto const requirements = d.device().getImageMemoryRequirements(m_storage.image);
	m_data.queueFlags = info.queueFlags;
	VmaAllocationInfo allocationInfo;
	vmaGetAllocationInfo(memory.m_allocator, m_data.handle, &allocationInfo);
	m_data.alloc = {vk::DeviceMemory(allocationInfo.deviceMemory), allocationInfo.offset, allocationInfo.size};
	m_storage.allocatedSize = requirements.size;
	m_data.mode = imageInfo.sharingMode;
	memory.m_allocations[(std::size_t)type].fetch_add(m_storage.allocatedSize);
}

Image::Image(Image&& rhs) : Resource(rhs.m_memory), m_storage(std::exchange(rhs.m_storage, Storage())) {
	m_data = std::exchange(rhs.m_data, Data());
}

Image& Image::operator=(Image&& rhs) {
	if (&rhs != this) {
		destroy();
		m_data = std::exchange(rhs.m_data, Data());
		m_storage = std::exchange(rhs.m_storage, Storage());
		m_memory = rhs.m_memory;
	}
	return *this;
}

Image::~Image() {
	destroy();
}

void Image::destroy() {
	if (!Device::default_v(m_storage.image)) {
		Memory& m = m_memory;
		m.m_allocations[(std::size_t)type].fetch_sub(m_storage.allocatedSize);
		auto del = [a = m.m_allocator, i = m_storage.image, h = m_data.handle]() { vmaDestroyImage(a, static_cast<VkImage>(i), h); };
		m.m_device.get().defer(del);
	}
}
} // namespace le::graphics
