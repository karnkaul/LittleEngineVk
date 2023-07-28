#pragma once
#include <spaced/graphics/mesh.hpp>
#include <spaced/resources/asset.hpp>

namespace spaced {
class SkeletonAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"SkeletonAsset"};

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	graphics::Skeleton skeleton{};
};
} // namespace spaced
