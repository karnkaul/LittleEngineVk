#include <core/log_channel.hpp>
#include <graphics/buffer.hpp>
#include <graphics/context/device.hpp>

namespace le::graphics {
Buffer::Buffer(not_null<Memory*> memory) noexcept : m_memory(memory) { m_buffer.resource = vk::Buffer(); }

Buffer::Buffer(not_null<Memory*> memory, CreateInfo const& info) : m_memory(memory) {
	Device& device = *memory->m_device;
	vk::BufferCreateInfo bufferInfo;
	m_buffer.size = bufferInfo.size = info.size;
	bufferInfo.usage = info.usage;
	bufferInfo.sharingMode = Memory::sharingMode(device.queues(), info.qcaps);
	bufferInfo.queueFamilyIndexCount = 1U;
	u32 const family = device.queues().primary().family();
	bufferInfo.pQueueFamilyIndices = &family;
	if (auto buf = m_memory->makeBuffer(info, bufferInfo)) {
		m_buffer = *buf;
		m_data.usage = info.usage;
		m_data.type = info.vmaUsage == VMA_MEMORY_USAGE_GPU_ONLY ? Buffer::Type::eGpuOnly : Buffer::Type::eCpuToGpu;
	} else {
		logE(LC_LibUser, "[{}] Failed to create Buffer!", g_name);
		EXPECT(false);
	}
}

Buffer::~Buffer() {
	if (!Device::default_v(buffer())) { m_memory->defer(m_buffer); }
}

void const* Buffer::map() {
	if (m_data.type != Buffer::Type::eCpuToGpu) {
		logE(LC_LibUser, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_buffer.data && m_buffer.size > 0) { m_memory->map(m_buffer); }
	return mapped();
}

bool Buffer::unmap() {
	if (m_buffer.data) {
		m_memory->unmap(m_buffer);
		return true;
	}
	return false;
}

bool Buffer::write(void const* data, vk::DeviceSize size, vk::DeviceSize offset) {
	if (m_data.type != Buffer::Type::eCpuToGpu) {
		logE(LC_LibUser, "[{}] Attempt to write to GPU-only Buffer!", g_name);
		return false;
	}
	EXPECT(size + offset <= m_buffer.size);
	if (size + offset > m_buffer.size) { return false; }
	if (!Device::default_v(m_buffer.alloc.memory) && !Device::default_v(buffer())) {
		if (size == 0) { size = m_buffer.size - offset; }
		if (auto ptr = map()) {
			char* start = (char*)ptr + offset;
			std::memcpy(start, data, (std::size_t)size);
			++m_data.writeCount;
			return true;
		}
	}
	return false;
}

void Buffer::exchg(Buffer& lhs, Buffer& rhs) noexcept {
	std::swap(lhs.m_data, rhs.m_data);
	std::swap(lhs.m_buffer, rhs.m_buffer);
	std::swap(lhs.m_memory, rhs.m_memory);
}
} // namespace le::graphics
