#pragma once
#include <levk/asset.hpp>
#include <levk/core/ptr.hpp>
#include <levk/image_file.hpp>

namespace levk {
class ImageAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"ImageAsset"};

	ImageFile image{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
