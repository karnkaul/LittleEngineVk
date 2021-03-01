#include <graphics/descriptor_set.hpp>
#include <graphics/shader_buffer.hpp>

namespace le::graphics {
ShaderBuffer::ShaderBuffer(VRAM& vram, std::string name, CreateInfo const& info) : m_vram(vram) {
	m_storage.name = std::move(name);
	m_storage.type = info.type;
	m_storage.usage = usage(info.type);
	m_storage.rotateCount = info.rotateCount;
}

ShaderBuffer const& ShaderBuffer::update(DescriptorSet& out_set, u32 binding) const {
	ENSURE(!m_storage.buffers.empty() && m_storage.elemSize > 0, "Empty buffer!");
	if (m_storage.buffers.size() > 1) {
		std::vector<Ref<Buffer const>> vec;
		vec.reserve(m_storage.buffers.size());
		for (RingBuffer<Buffer> const& rb : m_storage.buffers) {
			vec.push_back(rb.get());
		}
		out_set.updateBuffers(binding, vec, m_storage.elemSize);
	} else {
		Ref<Buffer const> buffer(m_storage.buffers.front().get());
		out_set.updateBuffers(binding, buffer, m_storage.elemSize);
	}
	return *this;
}

ShaderBuffer& ShaderBuffer::swap() {
	for (auto& rb : m_storage.buffers) {
		rb.next();
	}
	return *this;
}

void ShaderBuffer::resize(std::size_t size, std::size_t count) {
	if (size != m_storage.elemSize || count != m_storage.buffers.size()) {
		m_storage.elemSize = size;
		m_storage.buffers.clear();
		m_storage.buffers.reserve(count);
		for (std::size_t i = m_storage.buffers.size(); i < count; ++i) {
			RingBuffer<Buffer> buffer;
			io::Path prefix(m_storage.name);
			if (count > 1) {
				prefix += "[";
				prefix += std::to_string(i);
				prefix += "]";
			}
			for (u32 j = 0; j < m_storage.rotateCount; ++j) {
				buffer.ts.push_back(m_vram.get().createBO((prefix / std::to_string(j)).generic_string(), m_storage.elemSize, m_storage.usage, true));
			}
			m_storage.buffers.push_back(std::move(buffer));
		}
	}
}
} // namespace le::graphics
