#pragma once
#include <spaced/vfs/uri.hpp>
#include <vulkan/vulkan.hpp>
#include <unordered_map>

namespace spaced::graphics {
class ShaderCache {
  public:
	[[nodiscard]] auto load(Uri const& uri) -> vk::ShaderModule;

	auto clear_shaders() -> void;

  private:
	std::unordered_map<Uri, vk::UniqueShaderModule, Uri::Hasher> m_modules{};
};
} // namespace spaced::graphics
