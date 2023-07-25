#pragma once
#include <spaced/engine/graphics/mesh.hpp>
#include <spaced/engine/resources/asset.hpp>

namespace spaced {
class MeshAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"MeshAsset"};

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	graphics::Mesh mesh{};
};
} // namespace spaced
