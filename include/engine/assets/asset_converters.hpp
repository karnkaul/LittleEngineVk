#pragma once
#include <core/io/converters.hpp>
#include <core/io/path.hpp>
#include <graphics/glyph.hpp>

namespace le {
struct BitmapFontInfo {
	io::Path atlasID;
	std::vector<graphics::Glyph> glyphs;
	u32 sizePt{};
};

namespace io {
template <>
struct Jsonify<graphics::Glyph> : JsonHelper {
	dj::json operator()(graphics::Glyph const& glyph) const;
	graphics::Glyph operator()(dj::json const& json) const;
};

template <>
struct Jsonify<BitmapFontInfo> : JsonHelper {
	dj::json operator()(BitmapFontInfo const& info) const;
	BitmapFontInfo operator()(dj::json const& json) const;
};
} // namespace io
} // namespace le
