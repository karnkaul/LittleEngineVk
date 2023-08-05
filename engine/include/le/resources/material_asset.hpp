#pragma once
#include <le/graphics/material.hpp>
#include <le/resources/asset.hpp>

namespace le {
class MaterialAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"MaterialAsset"};

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	std::unique_ptr<graphics::Material> material{};
};
} // namespace le
