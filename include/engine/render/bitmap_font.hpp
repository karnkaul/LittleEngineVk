#pragma once
#include <core/io/path.hpp>
#include <graphics/geometry.hpp>
#include <graphics/glyph.hpp>
#include <graphics/texture.hpp>

namespace le {
namespace io {
class Media;
}

class BitmapFont {
  public:
	using Extent2D = graphics::Extent2D;
	using VRAM = graphics::VRAM;
	using Texture = graphics::Texture;
	using Sampler = graphics::Sampler;
	using Glyph = graphics::Glyph;

	struct CreateInfo;

	bool make(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info);

	bool valid() const noexcept { return m_storage.atlas.has_value(); }
	Texture const& atlas() const;

	graphics::GlyphMap const& glyphs() const noexcept { return m_storage.glyphs; }
	Extent2D atlasSize() const noexcept { return atlas().extent(); }

  private:
	struct {
		graphics::GlyphMap glyphs;
		std::optional<Texture> atlas;
	} m_storage;
};

struct BitmapFont::CreateInfo {
	std::optional<vk::Format> forceFormat;
	Span<Glyph const> glyphs;
	graphics::ImageData atlas;
};

inline BitmapFont::Texture const& BitmapFont::atlas() const {
	ENSURE(valid(), "BitmapFont Texture not valid");
	return *m_storage.atlas;
}
} // namespace le
