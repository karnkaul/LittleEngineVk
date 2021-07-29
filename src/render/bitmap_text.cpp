#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/primitive.hpp>

namespace le {
void BitmapText::make(not_null<graphics::VRAM*> vram, Type type) { mesh = graphics::Mesh(vram, type); }

bool BitmapText::set(BitmapFont const& font, std::string_view str) { return set(font.glyphs(), font.atlas().data().size, str); }

bool BitmapText::set(Span<graphics::Glyph const> glyphs, glm::ivec2 atlas, std::string_view str) {
	text.text = str;
	if (mesh) { return mesh->construct(text.generate(glyphs, atlas)); }
	return false;
}

Primitive const& BitmapText::update(BitmapFont const& font) { return update(font.atlas()); }

Primitive const& BitmapText::update(graphics::Texture const& atlas) {
	if (mesh) {
		prim.material.map_Kd = &atlas;
		prim.material.map_d = &atlas;
		prim.mesh = &*mesh;
	}
	return prim;
}
} // namespace le
