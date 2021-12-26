#pragma once
#include <graphics/font/face.hpp>
#include <graphics/texture_atlas.hpp>

namespace le::graphics {
class FontAtlas {
  public:
	using Height = FontFace::Height;
	using CreateInfo = TextureAtlas::CreateInfo;
	using Outcome = TextureAtlas::Outcome;
	using Result = TextureAtlas::Result;

	FontAtlas(not_null<VRAM*> vram, CreateInfo const& info);

	bool load(Span<std::byte const> ttf, Height height = {}) noexcept;

	Result build(CommandBuffer const& cb, Codepoint cp, bool rebuild = false);
	Glyph glyph(Codepoint cp) const noexcept;
	bool contains(Codepoint cp) const noexcept { return m_atlas.contains(cp); }

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
