#pragma once
#include <spaced/graphics/texture.hpp>
#include <spaced/resources/asset.hpp>

namespace spaced {
class TextureAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"TextureAsset"};

	graphics::Texture texture{};

	[[nodiscard]] auto try_load(dj::Json const& json) -> bool;
	[[nodiscard]] auto try_load(std::span<std::uint8_t const> bytes, graphics::ColourSpace colour_space) -> bool;

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;
};
} // namespace spaced
