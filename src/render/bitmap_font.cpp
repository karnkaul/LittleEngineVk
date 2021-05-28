#include <core/io/reader.hpp>
#include <engine/render/bitmap_font.hpp>

namespace le {
bool BitmapFont::create(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo const& info) {
	decltype(m_storage) storage;
	for (Glyph const& glyph : info.glyphs) { storage.glyphs[(std::size_t)glyph.ch] = glyph; }
	storage.atlas.emplace(graphics::Texture(vram));
	graphics::Texture::CreateInfo tci;
	tci.sampler = sampler.sampler();
	tci.data = graphics::Texture::Img{info.atlas.begin(), info.atlas.end()};
	tci.forceFormat = info.forceFormat;
	if (!storage.atlas->construct(tci)) { return false; }
	m_storage = std::move(storage);
	return true;
}
} // namespace le
