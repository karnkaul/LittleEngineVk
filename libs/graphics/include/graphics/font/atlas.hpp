#pragma once
#include <graphics/font/face.hpp>
#include <graphics/texture_atlas.hpp>

namespace le::graphics {
class FontAtlas {
  public:
	using Size = FontFace::Size;
	struct CreateInfo;

	FontAtlas(not_null<VRAM*> vram, CreateInfo const& info);

	bool load(CommandBuffer const& cb, Span<std::byte const> ttf, Size size = CharSize()) noexcept;

	Glyph const& build(CommandBuffer const& cb, Codepoint cp, bool rebuild = false);
	Extent2D extent(CommandBuffer const& cb, std::string_view line);

	TextureAtlas const& atlas() const noexcept { return m_atlas; }
	Texture const& texture() const noexcept { return m_atlas.texture(); }
	bool write(Geometry& out, glm::vec3 origin, Glyph const& glyph) const;

  private:
	TextureAtlas m_atlas;
	std::unordered_map<Codepoint, Glyph, std::hash<Codepoint::type>> m_glyphs;
	FontFace m_face;
	TextureAtlas::CreateInfo m_atlasInfo;
	not_null<VRAM*> m_vram;
};

struct FontAtlas::CreateInfo {
	TextureAtlas::CreateInfo atlas;
};
} // namespace le::graphics
