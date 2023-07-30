#pragma once
#include <spaced/graphics/dynamic_atlas.hpp>
#include <spaced/graphics/font/glyph.hpp>
#include <spaced/graphics/font/glyph_page.hpp>

namespace spaced::graphics {
struct FontAtlasCreateInfo {
	static constexpr std::pair<Codepoint, Codepoint> ascii_range_v{Codepoint::eAsciiFirst, Codepoint::eAsciiLast};

	std::span<std::pair<Codepoint, Codepoint> const> codepoint_ranges{&ascii_range_v, 1};
	TextHeight height{TextHeight::eDefault};
};

class FontAtlas {
  public:
	using CreateInfo = FontAtlasCreateInfo;

	explicit FontAtlas(NotNull<GlyphSlot::Factory*> slot_factory, CreateInfo create_info = {});

	[[nodiscard]] auto glyph_for(Codepoint codepoint) const -> Glyph;

	[[nodiscard]] auto get_dynamic_atlas() const -> DynamicAtlas const& { return m_atlas; }
	[[nodiscard]] auto get_glyph_page() const -> GlyphPage const& { return m_page; }
	[[nodiscard]] auto get_texture() const -> Texture const& { return m_atlas.get_texture(); }

  private:
	std::unordered_map<Codepoint, Glyph> m_glyphs{};
	DynamicAtlas m_atlas{DynamicAtlas::CreateInfo{.mip_map = false}};
	GlyphPage m_page;
};
} // namespace spaced::graphics
