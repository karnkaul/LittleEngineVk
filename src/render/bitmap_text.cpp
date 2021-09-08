#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/prop.hpp>
#include <graphics/glyph_pen.hpp>

namespace le {
graphics::Geometry TextGen::operator()(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text) const {
	graphics::GlyphPen pen(&glyphs, atlas, size, position, colour);
	return pen.generate(text, nLinePad, align);
}

Prop TextGen::prop(graphics::Mesh const& mesh, graphics::Texture const& atlas) const noexcept {
	Prop ret;
	ret.material.map_Kd = &atlas;
	ret.material.map_d = &atlas;
	ret.material.Tf = colour;
	ret.mesh = &mesh;
	return ret;
}

Prop const& TextMesh::prop() const noexcept {
	if (font) { prop_ = gen.prop(mesh, font->atlas()); }
	return prop_;
}

BitmapText::BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram) : m_mesh(vram, font), m_font(font) {}

void BitmapText::set(std::string_view text) { m_mesh.mesh.construct(m_mesh.gen(m_font->atlasSize(), m_font->glyphs(), text)); }
} // namespace le
