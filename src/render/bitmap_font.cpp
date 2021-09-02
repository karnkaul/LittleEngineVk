#include <core/io/reader.hpp>
#include <engine/render/bitmap_font.hpp>

namespace le {
bool BitmapFont::make(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info) {
	decltype(m_storage) storage;
	storage.glyphs = info.glyphs;
	storage.atlas.emplace(graphics::Texture(vram));
	graphics::Texture::CreateInfo tci;
	tci.sampler = sampler.sampler();
	tci.data = std::move(info.atlas);
	tci.forceFormat = info.forceFormat;
	if (!storage.atlas->construct(tci)) { return false; }
	m_storage = std::move(storage);
	return true;
}
} // namespace le
