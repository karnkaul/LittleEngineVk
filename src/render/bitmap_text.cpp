#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/prop.hpp>
#include <graphics/glyph_pen.hpp>

namespace le {
graphics::Geometry TextGen::operator()(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text) const {
	graphics::GlyphPen pen(&glyphs, atlas, size, position, colour);
	return graphics::GlyphPen::Scribe{}(pen, text, align);
}

Prop TextGen::prop(graphics::Mesh const& mesh, graphics::Texture const& atlas) {
	Prop ret;
	ret.material.map_Kd = &atlas;
	ret.material.map_d = &atlas;
	ret.mesh = &mesh;
	return ret;
}

BitmapText::BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type) : m_mesh(vram, type), m_font(font) {}

void BitmapText::set(std::string_view text) { m_mesh.construct(m_gen(m_font->atlasSize(), m_font->glyphs(), text)); }

Span<Prop const> BitmapText::props() const {
	m_prop = m_gen.prop(m_mesh, m_font->atlas());
	return m_prop;
}
} // namespace le
