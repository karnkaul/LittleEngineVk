#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/prop.hpp>
#include <graphics/glyph_pen.hpp>

namespace le {
void TextMesh::make(not_null<graphics::VRAM*> vram, Type type) { mesh = graphics::Mesh(vram, type); }

bool TextMesh::set(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text) {
	if (mesh) {
		graphics::GlyphPen pen(&glyphs, atlas, size, position, colour);
		return mesh->construct(pen.generate(text, align));
	}
	return false;
}

Span<Prop const> TextMesh::prop(graphics::Texture const& atlas) const {
	if (mesh) {
		prop_.material.map_Kd = &atlas;
		prop_.material.map_d = &atlas;
		prop_.mesh = &*mesh;
		return prop_;
	}
	return {};
}

BitmapText::BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type) : m_font(font) { m_text.make(vram, type); }

bool BitmapText::set(std::string_view text) {
	if (m_font) {
		graphics::GlyphPen pen(&m_font->glyphs(), m_font->atlasSize(), m_text.size, m_text.position, m_text.colour);
		return m_text.mesh->construct(pen.generate(text, m_text.align));
	}
	return false;
}

Span<Prop const> BitmapText::props() const { return m_font ? m_text.prop(m_font->atlas()) : Span<Prop const>(); }
} // namespace le
