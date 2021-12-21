#include <graphics/bitmap_font.hpp>

namespace le::graphics {
bool BitmapFont::make(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info) {
	decltype(m_storage) storage;
	storage.glyphs = graphics::BitmapGlyphMap(info.glyphs);
	storage.atlas.emplace(vram, sampler.sampler());
	if (!storage.atlas->construct(info.atlas, graphics::Texture::Payload::eColour, info.forceFormat.value_or(graphics::Image::srgb_v))) { return false; }
	m_storage = std::move(storage);
	return true;
}
} // namespace le::graphics
