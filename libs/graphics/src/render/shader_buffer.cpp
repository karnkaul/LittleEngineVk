#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/shader_buffer.hpp>

namespace le::graphics {
ShaderBuffer::ShaderBuffer(VRAM& vram, CreateInfo const& info) : m_vram(&vram) {
	m_storage.type = info.type;
	m_storage.usage = usage(info.type);
	m_storage.buffering = info.buffering;
}

ShaderBuffer& ShaderBuffer::write(void const* data, std::size_t size, std::size_t offset) {
	m_storage.buffers.front().get().write(data, (vk::DeviceSize)size, (vk::DeviceSize)offset);
	return *this;
}

ShaderBuffer const& ShaderBuffer::update(DescriptorSet& out_set, u32 binding) const {
	ENSURE(valid(), "Invalid ShaderBuffer instance");
	ENSURE(!m_storage.buffers.empty() && m_storage.elemSize > 0, "Empty buffer!");
	if (m_storage.buffers.size() > 1) {
		std::vector<Ref<Buffer const>> vec;
		vec.reserve(m_storage.buffers.size());
		for (RingBuffer<Buffer> const& rb : m_storage.buffers) { vec.push_back(rb.get()); }
		out_set.update(binding, vec, m_storage.type);
	} else {
		out_set.update(binding, m_storage.buffers.front().get(), m_storage.type);
	}
	return *this;
}

ShaderBuffer& ShaderBuffer::swap() {
	for (auto& rb : m_storage.buffers) { rb.next(); }
	return *this;
}

void ShaderBuffer::resize(std::size_t size, std::size_t count) {
	ENSURE(valid(), "Invalid ShaderBuffer instance");
	if (size != m_storage.elemSize || count != m_storage.buffers.size()) {
		m_storage.elemSize = size;
		m_storage.buffers.clear();
		m_storage.buffers.reserve(count);
		for (std::size_t i = m_storage.buffers.size(); i < count; ++i) {
			RingBuffer<Buffer> buffer;
			for (Buffering j{}; j < m_storage.buffering; ++j.value) { buffer.ts.push_back(m_vram->makeBuffer(m_storage.elemSize, m_storage.usage, true)); }
			m_storage.buffers.push_back(std::move(buffer));
		}
	}
}
} // namespace le::graphics
