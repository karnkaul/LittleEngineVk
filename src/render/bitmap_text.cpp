#include <engine/render/bitmap_font.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/prop.hpp>

namespace le {
void BitmapText::make(not_null<graphics::VRAM*> vram, Type type) { mesh = graphics::Mesh(vram, type); }

bool BitmapText::set(BitmapFont const& font, std::string_view text) { return set(font.glyphs(), font.atlas().data().size, text); }

bool BitmapText::set(Span<graphics::Glyph const> glyphs, glm::ivec2 atlas, std::string_view text) {
	if (mesh) { return mesh->construct(factory.generate(glyphs, text, atlas)); }
	return false;
}

Span<Prop const> BitmapText::prop(BitmapFont const& font) const { return prop(font.atlas()); }

Span<Prop const> BitmapText::prop(graphics::Texture const& atlas) const {
	if (mesh) {
		prop_.material.map_Kd = &atlas;
		prop_.material.map_d = &atlas;
		prop_.mesh = &*mesh;
		return prop_;
	}
	return {};
}
} // namespace le
