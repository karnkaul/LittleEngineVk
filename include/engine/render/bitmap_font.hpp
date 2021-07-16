#pragma once
#include <core/io/path.hpp>
#include <graphics/geometry.hpp>
#include <graphics/text_factory.hpp>
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
	using Glyph = graphics::Glyph;

	struct CreateInfo;

	static constexpr std::size_t max_glyphs = maths::max<u8>();

	bool create(not_null<VRAM*> vram, Sampler const& sampler, CreateInfo info);

	bool valid() const noexcept;
	Texture const& atlas() const;
	Span<Glyph const> glyphs() const noexcept;

  private:
	struct {
		std::array<Glyph, max_glyphs> glyphs;
		std::optional<Texture> atlas;
	} m_storage;
};

struct BitmapFont::CreateInfo {
	std::optional<vk::Format> forceFormat;
	Span<Glyph const> glyphs;
	graphics::Texture::Img atlas;
};

inline bool BitmapFont::valid() const noexcept { return m_storage.atlas.has_value(); }
inline BitmapFont::Texture const& BitmapFont::atlas() const {
	ensure(m_storage.atlas.has_value(), "Empty atlas");
	return *m_storage.atlas;
}
inline Span<BitmapFont::Glyph const> BitmapFont::glyphs() const noexcept { return m_storage.glyphs; }
} // namespace le
