#pragma once
#include <spaced/resources/asset.hpp>
#include <vulkan/vulkan.hpp>

namespace spaced {
class ShaderAsset : public Asset {
  public:
	vk::UniqueShaderModule module{};

	[[nodiscard]] auto type_name() const -> std::string_view final { return "ShaderAsset"; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;
};
} // namespace spaced
