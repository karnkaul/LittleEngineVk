#pragma once
#include <levk/asset.hpp>
#include <levk/skeleton.hpp>

namespace levk {
class SkeletalAnimationAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"SkeletalAnimationAsset"};

	SkeletalAnimation animation{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
