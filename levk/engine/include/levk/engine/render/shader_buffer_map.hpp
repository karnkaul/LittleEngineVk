#pragma once
#include <levk/engine/engine.hpp>
#include <levk/graphics/render/shader_buffer.hpp>

namespace le {
class ShaderBufferMap {
  public:
	using Buffer = graphics::ShaderBuffer;

	ShaderBufferMap(Engine::Service engine) noexcept;

	Buffer& make(Hash id, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	Buffer& get(Hash id);
	void swap();

  private:
	std::unordered_map<Hash, Buffer> m_shaderBuffers;
	Engine::Service m_engine;
	graphics::Buffering m_buffering = graphics::Buffering::eDouble;
};
} // namespace le
