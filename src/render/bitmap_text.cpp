#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/prop.hpp>

namespace le {
void BitmapTextMesh::make(not_null<graphics::VRAM*> vram, Type type) { mesh = graphics::Mesh(vram, type); }

bool BitmapTextMesh::set(BitmapFont const& font, Glyphs const& glyphs, std::string_view text) {
	if (mesh) {
		graphics::BitmapGlyphPen pen(&glyphs, font.atlasSize(), size, position, colour);
		return mesh->construct(pen.generate(text, align));
	}
	return false;
}

Span<Prop const> BitmapTextMesh::prop(BitmapFont const& font) const { return prop(font.atlas()); }

Span<Prop const> BitmapTextMesh::prop(graphics::Texture const& atlas) const {
	if (mesh) {
		prop_.material.map_Kd = &atlas;
		prop_.material.map_d = &atlas;
		prop_.mesh = &*mesh;
		return prop_;
	}
	return {};
}

BitmapText::BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type) : m_font(font) {
	m_text.make(vram, type);
	m_glyphs = m_font->glyphs();
}

bool BitmapText::set(std::string_view text) {
	if (m_font) {
		graphics::BitmapGlyphPen pen(&m_glyphs, m_font->atlasSize(), m_text.size, m_text.position, m_text.colour);
		return m_text.mesh->construct(pen.generate(text, m_text.align));
	}
	return false;
}

Span<Prop const> BitmapText::props() const { return m_font ? m_text.prop(*m_font) : Span<Prop const>(); }
} // namespace le
