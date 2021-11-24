#include <engine/assets/asset_converters.hpp>

namespace le::io {
dj::json Jsonify<graphics::Glyph>::operator()(graphics::Glyph const& glyph) const {
	dj::json ret;
	insert(ret, "x", glyph.topLeft.x);
	insert(ret, "y", glyph.topLeft.y);
	insert(ret, "width", glyph.size.x);
	insert(ret, "height", glyph.size.y);
	insert(ret, "originX", glyph.origin.x);
	insert(ret, "originY", glyph.origin.y);
	insert(ret, "advance", glyph.advance);
	return ret;
}

graphics::Glyph Jsonify<graphics::Glyph>::operator()(dj::json const& json) const {
	graphics::Glyph ret;
	set(ret.topLeft.x, json.get("x"));
	set(ret.topLeft.y, json.get("y"));
	set(ret.size.x, json.get("width"));
	set(ret.size.y, json.get("height"));
	set(ret.origin.x, json.get("originX"));
	set(ret.origin.y, json.get("originY"));
	set(ret.advance, json.get("advance"));
	return ret;
}

dj::json Jsonify<BitmapFontInfo>::operator()(BitmapFontInfo const& info) const {
	dj::json ret;
	insert(ret, "sheetID", info.atlasURI.generic_string());
	insert(ret, "size", info.sizePt);
	dj::json glyphs;
	for (auto const& glyph : info.glyphs) {
		char const ch = static_cast<char>(glyph.codepoint);
		glyphs.insert(std::string(1, ch), to(glyph));
	}
	return ret;
}

BitmapFontInfo Jsonify<BitmapFontInfo>::operator()(dj::json const& json) const {
	BitmapFontInfo ret;
	ret.atlasURI = json.get_as<std::string>("sheetID");
	set(ret.sizePt, json.get("size"));
	for (auto const& [codepoint, glyph] : json.get_as<dj::map_t>("glyphs")) {
		if (codepoint.size() == 1) {
			ret.glyphs.push_back(to<graphics::Glyph>(*glyph));
			ret.glyphs.back().codepoint = static_cast<u32>(codepoint[0]);
		}
	}
	return ret;
}
} // namespace le::io
