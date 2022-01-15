#include <levk/core/log_channel.hpp>
#include <levk/graphics/buffer.hpp>
#include <levk/graphics/context/device.hpp>

namespace le::graphics {
Buffer::Buffer(not_null<Memory*> memory, CreateInfo const& info) : m_memory(memory) {
	Device& device = *memory->m_device;
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = info.size;
	bufferInfo.usage = info.usage;
	bufferInfo.sharingMode = Memory::sharingMode(device.queues(), info.qcaps);
	bufferInfo.queueFamilyIndexCount = 1U;
	u32 const family = device.queues().primary().family();
	bufferInfo.pQueueFamilyIndices = &family;
	if (auto buf = m_memory->makeBuffer(info, bufferInfo)) {
		m_buffer = {*buf, m_memory->m_device, m_memory};
		m_data.usage = info.usage;
		m_data.type = info.vmaUsage == VMA_MEMORY_USAGE_GPU_ONLY ? Buffer::Type::eGpuOnly : Buffer::Type::eCpuToGpu;
	} else {
		logE(LC_LibUser, "[{}] Failed to create Buffer!", g_name);
		EXPECT(false);
	}
}

void const* Buffer::map() {
	if (m_data.type != Buffer::Type::eCpuToGpu) {
		logE(LC_LibUser, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_buffer.get().data && m_buffer.get().size > 0) { m_memory->map(m_buffer.get()); }
	return mapped();
}

bool Buffer::unmap() {
	if (m_buffer.get().data) {
		m_memory->unmap(m_buffer.get());
		return true;
	}
	return false;
}

bool Buffer::write(void const* data, vk::DeviceSize size, vk::DeviceSize offset) {
	if (m_data.type != Buffer::Type::eCpuToGpu) {
		logE(LC_LibUser, "[{}] Attempt to write to GPU-only Buffer!", g_name);
		return false;
	}
	EXPECT(size + offset <= m_buffer.get().size);
	if (size + offset > m_buffer.get().size) { return false; }
	if (!Device::default_v(m_buffer.get().alloc.memory) && !Device::default_v(buffer())) {
		if (size == 0) { size = m_buffer.get().size - offset; }
		if (auto ptr = map()) {
			char* start = (char*)ptr + offset;
			std::memcpy(start, data, (std::size_t)size);
			++m_data.writeCount;
			return true;
		}
	}
	return false;
}
} // namespace le::graphics
