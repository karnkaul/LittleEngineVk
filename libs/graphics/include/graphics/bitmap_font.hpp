#pragma once
#include <core/utils/error.hpp>
#include <graphics/bitmap_glyph.hpp>
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
class BitmapFont {
  public:
	struct CreateInfo;

	bool make(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info);

	bool valid() const noexcept { return m_storage.atlas.has_value(); }
	Texture const& atlas() const;

	graphics::BitmapGlyphMap const& glyphs() const noexcept { return m_storage.glyphs; }
	Extent2D atlasSize() const noexcept { return atlas().image().extent2D(); }

  private:
	struct {
		BitmapGlyphMap glyphs;
		std::optional<Texture> atlas;
	} m_storage;
};

struct BitmapFont::CreateInfo {
	std::optional<vk::Format> forceFormat;
	Span<BitmapGlyph const> glyphs;
	ImageData atlas;
};

inline Texture const& BitmapFont::atlas() const {
	ENSURE(valid(), "BitmapFont Texture not valid");
	return *m_storage.atlas;
}
} // namespace le::graphics
