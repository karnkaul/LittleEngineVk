#pragma once
#include <core/array_map.hpp>
#include <core/io/converters.hpp>
#include <core/io/path.hpp>
#include <engine/render/layer.hpp>
#include <graphics/bitmap_glyph.hpp>

namespace le {
struct BitmapFontInfo {
	io::Path atlasURI;
	std::vector<graphics::BitmapGlyph> glyphs;
	u32 sizePt{};
};

namespace io {
constexpr ArrayMap<PolygonMode, std::string_view, std::size_t(PolygonMode::eCOUNT_)> polygonModes = {{
	{PolygonMode::eFill, "fill"},
	{PolygonMode::eLine, "line"},
	{PolygonMode::ePoint, "point"},
}};

constexpr ArrayMap<Topology, std::string_view, std::size_t(Topology::eCOUNT_)> topologies = {{
	{Topology::ePointList, "point_list"},
	{Topology::eLineList, "line_list"},
	{Topology::eLineStrip, "line_strip"},
	{Topology::eTriangleList, "triangle_list"},
	{Topology::eTriangleStrip, "triangle_strip"},
	{Topology::eTriangleFan, "triangle_fan"},
}};

constexpr ArrayMap<RenderFlag, std::string_view, std::size_t(RenderFlag::eCOUNT_)> renderFlags = {{
	{RenderFlag::eDepthTest, "depth_test"},
	{RenderFlag::eDepthWrite, "depth_write"},
	{RenderFlag::eAlphaBlend, "alpha_blend"},
	{RenderFlag::eWireframe, "wireframe"},
}};

template <>
struct Jsonify<graphics::BitmapGlyph> : JsonHelper {
	dj::json operator()(graphics::BitmapGlyph const& glyph) const;
	graphics::BitmapGlyph operator()(dj::json const& json) const;
};

template <>
struct Jsonify<BitmapFontInfo> : JsonHelper {
	dj::json operator()(BitmapFontInfo const& info) const;
	BitmapFontInfo operator()(dj::json const& json) const;
};

template <>
struct Jsonify<RenderFlags> : JsonHelper {
	dj::json operator()(RenderFlags const& flags) const;
	RenderFlags operator()(dj::json const& json) const;
};

template <>
struct Jsonify<RenderLayer> : JsonHelper {
	dj::json operator()(RenderLayer const& layer) const;
	RenderLayer operator()(dj::json const& json) const;
};
} // namespace io
} // namespace le
