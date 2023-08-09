#pragma once
#include <le/graphics/primitive.hpp>
#include <le/resources/asset.hpp>

namespace le {
class PrimitiveAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"PrimitiveAsset"};

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	static auto bin_pack_to(std::vector<std::byte>& out, graphics::Geometry const& geometry) -> void;

	graphics::StaticPrimitive primitive{};
};
} // namespace le
