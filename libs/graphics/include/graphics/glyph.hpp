#pragma once
#include <unordered_map>
#include <core/maths.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>
#include <ktl/either.hpp>

namespace le::graphics {
struct Glyph {
	using Size = ktl::either<u32, f32>;

	glm::uvec2 size{};
	glm::ivec2 topLeft{};
	glm::ivec2 origin{};
	u32 advance{};
	u32 codepoint{};

	static constexpr glm::uvec2 bounds(Span<Glyph const> glyphs) noexcept;
	template <typename Map>
	static constexpr glm::uvec2 bounds(Map const& glyphs) noexcept;

	constexpr bool valid() const noexcept { return codepoint > 0; }
	constexpr glm::uvec2 maxBounds(glm::uvec2 rhs) const noexcept;

	template <typename T = char>
	constexpr T to() const noexcept {
		return static_cast<T>(codepoint);
	}
};

class GlyphMap {
  public:
	using Map = std::unordered_map<u32, Glyph>;

	GlyphMap() = default;
	GlyphMap(Span<Glyph const> glyphs) noexcept;

	Map const& map() const noexcept { return m_glyphs; }
	glm::uvec2 bounds() const noexcept { return m_bounds; }
	f32 lineHeight(f32 scale) const noexcept { return static_cast<f32>(m_bounds.y) * scale; }
	Glyph const& glyph(u32 codepoint) const noexcept;

	Glyph& operator[](u32 codepoint) { return m_glyphs[codepoint]; }
	bool add(u32 codepoint, Glyph const& glyph);
	void remove(u32 codepoint) noexcept;

  private:
	Map m_glyphs;
	glm::uvec2 m_bounds{};
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

constexpr glm::uvec2 Glyph::bounds(Span<Glyph const> glyphs) noexcept {
	glm::uvec2 ret{};
	for (Glyph const& glyph : glyphs) { ret = glyph.maxBounds(ret); }
	return ret;
}

template <typename Map>
constexpr glm::uvec2 Glyph::bounds(Map const& glyphs) noexcept {
	glm::uvec2 ret{};
	for (auto const& [_, glyph] : glyphs) { ret = glyph.maxBounds(ret); }
	return ret;
}

inline GlyphMap::GlyphMap(Span<Glyph const> glyphs) noexcept {
	for (Glyph const& glyph : glyphs) {
		if (glyph.valid()) {
			m_glyphs[glyph.codepoint] = glyph;
			m_bounds = glyph.maxBounds(m_bounds);
		}
	}
}

inline Glyph const& GlyphMap::glyph(u32 codepoint) const noexcept {
	if (auto it = m_glyphs.find(codepoint); it != m_glyphs.end()) { return it->second; }
	static constexpr Glyph fallback;
	return fallback;
}

inline bool GlyphMap::add(u32 codepoint, Glyph const& glyph) {
	if (glyph.valid()) {
		m_glyphs.insert_or_assign(codepoint, glyph);
		m_bounds = glyph.maxBounds(m_bounds);
		return true;
	}
	return false;
}

inline void GlyphMap::remove(u32 codepoint) noexcept {
	m_glyphs.erase(codepoint);
	m_bounds = Glyph::bounds(m_glyphs);
}
} // namespace le::graphics
