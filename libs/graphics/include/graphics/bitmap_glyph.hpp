#pragma once
#include <core/maths.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>

namespace le::graphics {
struct BitmapGlyph {
	u8 ch = '\0';
	glm::ivec2 st = glm::ivec2(0);
	glm::ivec2 uv = glm::ivec2(0);
	glm::ivec2 cell = glm::ivec2(0);
	glm::ivec2 offset = glm::ivec2(0);
	s32 xAdv = 0;
	bool blank = false;

	static constexpr glm::uvec2 glyphBounds(Span<BitmapGlyph const> glyphs) noexcept;

	constexpr bool valid() const noexcept { return !blank && ch != '\0'; }
};

class BitmapGlyphArray {
  public:
	static constexpr std::size_t size = maths::max<u8>();

	constexpr BitmapGlyphArray() = default;
	constexpr BitmapGlyphArray(Span<BitmapGlyph const> glyphs) noexcept;

	constexpr Span<BitmapGlyph const> glyphs() const noexcept { return m_glyphs; }
	constexpr glm::uvec2 bounds() const noexcept { return m_bounds; }
	constexpr f32 lineHeight(f32 scale) const noexcept { return static_cast<f32>(m_bounds.y) * scale; }
	constexpr BitmapGlyph const& glyph(u8 ch) const noexcept { return m_glyphs[static_cast<std::size_t>(ch)]; }

  private:
	BitmapGlyph m_glyphs[size] = {};
	glm::uvec2 m_bounds{};
};

// impl

constexpr glm::uvec2 BitmapGlyph::glyphBounds(Span<BitmapGlyph const> glyphs) noexcept {
	glm::uvec2 ret{};
	for (BitmapGlyph const& glyph : glyphs) {
		if (glyph.valid()) {
			if (glyph.cell.x > (s32)ret.x) { ret.x = (u32)glyph.cell.x; }
			if (glyph.cell.y > (s32)ret.y) { ret.y = (u32)glyph.cell.y; }
		}
	}
	return ret;
}

constexpr BitmapGlyphArray::BitmapGlyphArray(Span<BitmapGlyph const> glyphs) noexcept {
	for (BitmapGlyph const& glyph : glyphs) {
		if (glyph.valid()) {
			m_glyphs[static_cast<std::size_t>(glyph.ch)] = glyph;
			if (glyph.cell.x > (s32)m_bounds.x) { m_bounds.x = (u32)glyph.cell.x; }
			if (glyph.cell.y > (s32)m_bounds.y) { m_bounds.y = (u32)glyph.cell.y; }
		}
	}
}
} // namespace le::graphics
