#include <le/resources/font_asset.hpp>

namespace le {
auto FontAsset::try_load(Uri const& uri) -> bool {
	auto bytes = read_bytes(uri);
	if (bytes.empty()) { return false; }

	auto new_font = graphics::Font::try_make(bytes);
	if (!new_font) { return false; }

	font = std::move(new_font);
	return true;
}
} // namespace le
