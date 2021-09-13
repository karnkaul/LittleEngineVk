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
	using VRAM = graphics::VRAM;
	using Texture = graphics::Texture;
	using Sampler = graphics::Sampler;
	using Glyph = graphics::Glyph;

	struct CreateInfo;

	bool make(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info);

	bool valid() const noexcept { return m_storage.atlas.has_value(); }
	Texture const& atlas() const;

	graphics::GlyphMap const& glyphs() const noexcept { return m_storage.glyphs; }
	glm::uvec2 atlasSize() const noexcept { return atlas().data().size; }

  private:
	struct {
		graphics::GlyphMap glyphs;
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
