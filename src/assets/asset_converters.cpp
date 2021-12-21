#include <engine/assets/asset_converters.hpp>
#include <ktl/debug_trap.hpp>

namespace le::io {
dj::json Jsonify<graphics::BitmapGlyph>::operator()(graphics::BitmapGlyph const& glyph) const {
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

graphics::BitmapGlyph Jsonify<graphics::BitmapGlyph>::operator()(dj::json const& json) const {
	graphics::BitmapGlyph ret;
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
			ret.glyphs.push_back(to<graphics::BitmapGlyph>(*glyph));
			ret.glyphs.back().codepoint = static_cast<u32>(codepoint[0]);
		}
	}
	return ret;
}

dj::json Jsonify<RenderFlags>::operator()(RenderFlags const& flags) const {
	dj::json ret;
	if (flags == graphics::pflags_all) {
		ret.push_back(to<std::string>("all"));
		return ret;
	}
	if (flags.test(RenderFlag::eDepthTest)) { ret.push_back(to<std::string>("depth_test")); }
	if (flags.test(RenderFlag::eDepthWrite)) { ret.push_back(to<std::string>("depth_write")); }
	if (flags.test(RenderFlag::eAlphaBlend)) { ret.push_back(to<std::string>("alpha_blend")); }
	if (flags.test(RenderFlag::eWireframe)) { ret.push_back(to<std::string>("wireframe")); }
	return ret;
}

RenderFlags Jsonify<RenderFlags>::operator()(dj::json const& json) const {
	RenderFlags ret;
	if (json.is_array()) {
		for (auto const flag : json.as<std::vector<std::string_view>>()) {
			if (flag == "all") {
				ret = graphics::pflags_all;
				return ret;
			}
			if (flag == "depth_test") {
				ret.set(RenderFlag::eDepthTest);
			} else if (flag == "depth_write") {
				ret.set(RenderFlag::eDepthWrite);
			} else if (flag == "alpha_blend") {
				ret.set(RenderFlag::eAlphaBlend);
			} else if (flag == "wireframe") {
				ret.set(RenderFlag::eWireframe);
			}
		}
	}
	return ret;
}

dj::json Jsonify<RenderLayer>::operator()(RenderLayer const& layer) const {
	dj::json ret;
	insert(ret, "mode", polygonModes[layer.mode], "topology", topologies[layer.topology], "line_width", layer.lineWidth, "order", layer.order);
	ret.insert("flags", to(layer.flags));
	return ret;
}

RenderLayer Jsonify<RenderLayer>::operator()(dj::json const& json) const {
	RenderLayer ret;
	if (auto mode = json.get_as<std::string_view>("mode"); !mode.empty()) { ret.mode = polygonModes[mode]; }
	if (auto top = json.get_as<std::string_view>("topology"); !top.empty()) { ret.topology = topologies[top]; }
	ret.flags = to<RenderFlags>(json.get("flags"));
	ret.lineWidth = json.get_as<f32>("line_width", ret.lineWidth);
	ret.order = json.get_as<s64>("order");
	return ret;
}
} // namespace le::io
