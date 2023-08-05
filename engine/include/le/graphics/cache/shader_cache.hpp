#pragma once
#include <le/vfs/uri.hpp>
#include <vulkan/vulkan.hpp>
#include <unordered_map>

namespace le::graphics {
class ShaderCache {
  public:
	[[nodiscard]] auto load(Uri const& uri) -> vk::ShaderModule;

	[[nodiscard]] auto shader_count() const -> std::size_t { return m_modules.size(); }
	auto clear_shaders() -> void;

  private:
	std::unordered_map<Uri, vk::UniqueShaderModule, Uri::Hasher> m_modules{};
};
} // namespace le::graphics
