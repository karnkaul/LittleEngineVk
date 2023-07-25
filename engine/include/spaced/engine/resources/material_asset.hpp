#pragma once
#include <spaced/engine/graphics/material.hpp>
#include <spaced/engine/resources/asset.hpp>

namespace spaced {
class MaterialAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"MaterialAsset"};

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	[[nodiscard]] auto save_to(char const* path) const -> bool;

	std::unique_ptr<graphics::Material> material{};
};
} // namespace spaced
