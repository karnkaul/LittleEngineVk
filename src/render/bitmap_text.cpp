#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/primitive.hpp>

namespace le {
void BitmapText::make(not_null<graphics::VRAM*> vram, Type type) { mesh = graphics::Mesh(vram, type); }

bool BitmapText::set(BitmapFont const& font, std::string_view text) { return set(font.glyphs(), font.atlas().data().size, text); }

bool BitmapText::set(Span<graphics::Glyph const> glyphs, glm::ivec2 atlas, std::string_view text) {
	if (mesh) { return mesh->construct(factory.generate(glyphs, text, atlas)); }
	return false;
}

Span<Primitive const> BitmapText::primitive(BitmapFont const& font) const { return primitive(font.atlas()); }

Span<Primitive const> BitmapText::primitive(graphics::Texture const& atlas) const {
	if (mesh) {
		prim.material.map_Kd = &atlas;
		prim.material.map_d = &atlas;
		prim.mesh = &*mesh;
		return prim;
	}
	return {};
}
} // namespace le
