#pragma once
#include <levk/asset.hpp>
#include <levk/core/ptr.hpp>
#include <levk/texture.hpp>
#include <memory>

namespace levk {
class TextureAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"TextureAsset"};

	std::unique_ptr<ITexture> texture{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
