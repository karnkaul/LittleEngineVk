#pragma once
#include <spaced/graphics/font/font.hpp>
#include <spaced/resources/asset.hpp>

namespace spaced {
class FontAsset : public Asset {
  public:
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	std::optional<graphics::Font> font{};
};
} // namespace spaced
