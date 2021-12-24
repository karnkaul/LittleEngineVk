#include <engine/render/bitmap_text.hpp>
#include <graphics/bitmap_font.hpp>
#include <graphics/glyph_pen.hpp>

namespace le {
graphics::Geometry TextGen::operator()(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text) const {
	graphics::GlyphPen pen(&glyphs, atlas, size, position, colour);
	return pen.generate(text, nLinePad, align);
}

MeshView TextGen::mesh(graphics::MeshPrimitive const& primitive, graphics::Texture const& atlas) const noexcept {
	material.map_Kd = &atlas;
	material.map_d = &atlas;
	material.Tf = colour;
	return MeshObj{&primitive, &material};
}

MeshView TextGenMesh::mesh() const noexcept {
	if (font) { return gen.mesh(primitive, font->atlas()); }
	return {};
}

BitmapText::BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram) : m_textMesh(vram, font), m_font(font) {}

void BitmapText::set(std::string_view text, std::optional<f32> z) {
	if (z) { m_textMesh.gen.position.z = *z; }
	m_textMesh.primitive.construct(m_textMesh.gen(m_font->atlasSize(), m_font->glyphs(), text));
}
} // namespace le
