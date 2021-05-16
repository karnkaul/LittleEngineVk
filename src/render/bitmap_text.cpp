#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/primitive.hpp>

namespace le {
void BitmapText::create(not_null<graphics::VRAM*> vram, Type type) { mesh = graphics::Mesh(vram, type); }

bool BitmapText::set(BitmapFont const& font, std::string_view str) { return set(font.glyphs(), font.atlas().data().size, str); }

bool BitmapText::set(View<graphics::Glyph> glyphs, glm::ivec2 atlas, std::string_view str) {
	text.text = str;
	if (mesh) { return mesh->construct(text.generate(glyphs, atlas)); }
	return false;
}

Primitive BitmapText::primitive(BitmapFont const& font) const { return primitive(font.atlas()); }

Primitive BitmapText::primitive(graphics::Texture const& atlas) const {
	Primitive ret;
	if (mesh) {
		ret.material.map_Kd = &atlas;
		ret.material.map_d = &atlas;
		ret.mesh = &*mesh;
	}
	return ret;
}
} // namespace le
