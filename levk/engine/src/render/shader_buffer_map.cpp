#include <levk/engine/render/shader_buffer_map.hpp>
#include <levk/graphics/render/context.hpp>

namespace le {
ShaderBufferMap::ShaderBufferMap(Engine::Service engine) noexcept : m_engine(engine), m_buffering(m_engine.context().buffering()) {}

ShaderBufferMap::Buffer& ShaderBufferMap::make(Hash id, vk::DescriptorType type) {
	Buffer::CreateInfo const info{type, m_buffering};
	return m_shaderBuffers.insert_or_assign(id, graphics::ShaderBuffer(&m_engine.vram(), info)).first->second;
}

ShaderBufferMap::Buffer& ShaderBufferMap::get(Hash id) {
	if (auto const it = m_shaderBuffers.find(id); it != m_shaderBuffers.end()) { return it->second; }
	return make(id);
}

void ShaderBufferMap::swap() {
	for (auto& [_, buffer] : m_shaderBuffers) { buffer.swap(); }
}
} // namespace le
