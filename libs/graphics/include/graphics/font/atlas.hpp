#pragma once
#include <graphics/font/face.hpp>
#include <graphics/texture_atlas.hpp>

namespace le::graphics {
class FontAtlas {
  public:
	using Size = FontFace::Size;
	using CreateInfo = TextureAtlas::CreateInfo;

	FontAtlas(not_null<VRAM*> vram, CreateInfo const& info);

	bool load(CommandBuffer const& cb, Span<std::byte const> ttf, Size size = {}) noexcept;

	Glyph const& build(CommandBuffer const& cb, Codepoint cp, bool rebuild = false);

	FontFace const& face() const noexcept { return m_face; }
	TextureAtlas const& atlas() const noexcept { return m_atlas; }
	Texture const& texture() const noexcept { return m_atlas.texture(); }

	void lockSize(bool lock) noexcept { m_atlas.lockSize(lock); }

  private:
	TextureAtlas m_atlas;
	std::unordered_map<Codepoint, Glyph, std::hash<Codepoint::type>> m_glyphs;
	FontFace m_face;
	not_null<VRAM*> m_vram;
};
} // namespace le::graphics
