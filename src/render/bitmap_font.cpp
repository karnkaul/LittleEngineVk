#include <core/io/reader.hpp>
#include <dumb_json/djson.hpp>
#include <engine/render/bitmap_font.hpp>

namespace le {
bool BitmapFont::create(VRAM& vram, Sampler const& sampler, CreateInfo const& info) {
	decltype(m_storage) storage;
	for (Glyph const& glyph : info.glyphs) {
		storage.glyphs[(std::size_t)glyph.ch] = glyph;
	}
	storage.atlas.emplace(graphics::Texture(info.name + "/atlas", vram));
	graphics::Texture::CreateInfo tci;
	tci.sampler = sampler.sampler();
	tci.data = graphics::Texture::Img{{info.atlas.begin(), info.atlas.end()}};
	tci.format = info.format;
	if (!storage.atlas->construct(tci)) {
		return false;
	}
	m_storage = std::move(storage);
	return true;
}
} // namespace le
