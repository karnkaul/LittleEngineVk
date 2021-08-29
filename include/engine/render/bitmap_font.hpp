#pragma once
#include <core/io/path.hpp>
#include <graphics/bitmap_glyph.hpp>
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le {
namespace io {
class Reader;
}

class BitmapFont {
  public:
	using VRAM = graphics::VRAM;
	using Texture = graphics::Texture;
	using Sampler = graphics::Sampler;
	using Glyph = graphics::BitmapGlyph;

	struct CreateInfo;

	bool make(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info);

	bool valid() const noexcept { return m_storage.atlas.has_value(); }
	Texture const& atlas() const;

	Span<Glyph const> glyphs() const noexcept { return m_storage.glyphs.glyphs(); }

	glm::uvec2 atlasSize() const noexcept { return atlas().data().size; }
	glm::uvec2 bounds() const noexcept { return m_storage.glyphs.bounds(); }

  private:
	struct {
		graphics::BitmapGlyphArray glyphs;
		std::optional<Texture> atlas;
	} m_storage;
};

struct BitmapFont::CreateInfo {
	std::optional<vk::Format> forceFormat;
	Span<Glyph const> glyphs;
	graphics::Texture::Img atlas;
};

inline BitmapFont::Texture const& BitmapFont::atlas() const {
	ensure(valid());
	return *m_storage.atlas;
}
} // namespace le
