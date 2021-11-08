#pragma once
#include <core/codepoint.hpp>
#include <core/maths.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>
#include <ktl/either.hpp>
#include <unordered_map>

namespace le::graphics {

struct Glyph {
	using Size = ktl::either<u32, f32>;

	glm::uvec2 size{};
	glm::ivec2 topLeft{};
	glm::ivec2 origin{};
	Codepoint codepoint;
	u32 advance{};

	constexpr bool valid() const noexcept { return codepoint > Codepoint{}; }
	constexpr glm::uvec2 maxBounds(glm::uvec2 rhs) const noexcept;
	constexpr u32 maxHeight(u32 rhs) const noexcept;

	template <typename T = char>
	constexpr T to() const noexcept {
		return static_cast<T>(codepoint);
	}
};

class GlyphMap {
  public:
	using Map = std::unordered_map<Codepoint, Glyph, std::hash<Codepoint::type>>;

	GlyphMap() = default;
	template <typename Cont>
	explicit GlyphMap(Cont const& glyphs) noexcept;

	Glyph makeBlank(glm::vec2 nSize = {0.4f, 0.9f}) const noexcept;

	Map const& map() const noexcept { return m_glyphs; }
	glm::uvec2 bounds() const noexcept { return m_bounds; }
	f32 lineHeight(f32 scale) const noexcept { return static_cast<f32>(m_maxHeight) * scale; }
	Glyph const& glyph(u32 codepoint) const noexcept;
	Glyph const& blank() const noexcept;

	Glyph& operator[](u32 codepoint) { return m_glyphs[codepoint]; }
	bool add(u32 codepoint, Glyph const& glyph);
	void remove(u32 codepoint) noexcept;

  private:
	void refresh();

	Map m_glyphs;
	mutable Glyph m_blank;
	glm::uvec2 m_bounds{};
	u32 m_maxHeight{};
};

// impl

constexpr glm::uvec2 Glyph::maxBounds(glm::uvec2 rhs) const noexcept {
	glm::uvec2 ret = rhs;
	if (valid()) {
		if (size.x > ret.x) { ret.x = size.x; }
		if (size.y > ret.y) { ret.y = size.y; }
	}
	return ret;
}

constexpr u32 Glyph::maxHeight(u32 rhs) const noexcept {
	u32 ret = rhs;
	if (valid()) {
		u32 const height = u32(maths::abs(origin.y));
		if (height > ret) { ret = height; }
	}
	return ret;
}

template <typename Cont>
GlyphMap::GlyphMap(Cont const& glyphs) noexcept {
	for (Glyph const& glyph : glyphs) {
		if (glyph.valid()) {
			m_glyphs[glyph.codepoint] = glyph;
			m_bounds = glyph.maxBounds(m_bounds);
			m_maxHeight = glyph.maxHeight(m_maxHeight);
		}
	}
}
} // namespace le::graphics
