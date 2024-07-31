#pragma once
#include <levk/asset.hpp>
#include <levk/render_shader.hpp>
#include <vector>

namespace levk {
class ShaderAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"ShaderAsset"};

	[[nodiscard]] auto get_render_shader() const -> RenderShader { return m_spirv; }

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;

	std::vector<std::byte> m_spirv{};
};
} // namespace levk
