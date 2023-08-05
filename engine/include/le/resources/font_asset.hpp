#pragma once
#include <le/graphics/font/font.hpp>
#include <le/resources/asset.hpp>

namespace le {
class FontAsset : public Asset {
  public:
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	std::optional<graphics::Font> font{};
};
} // namespace le
