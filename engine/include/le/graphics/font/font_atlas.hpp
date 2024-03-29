#pragma once
#include <le/core/inclusive_range.hpp>
#include <le/graphics/dynamic_atlas.hpp>
#include <le/graphics/font/glyph.hpp>
#include <le/graphics/font/glyph_page.hpp>

namespace le::graphics {
struct FontAtlasCreateInfo {
	static constexpr InclusiveRange<Codepoint> ascii_range_v{Codepoint::eAsciiFirst, Codepoint::eAsciiLast};

	std::span<InclusiveRange<Codepoint> const> codepoint_ranges{&ascii_range_v, 1};
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
} // namespace le::graphics
