#include <graphics/glyph.hpp>

namespace le::graphics {
Glyph GlyphMap::makeBlank(glm::vec2 nSize) const noexcept {
	Glyph ret;
	ret.codepoint = codepoint_blank_v;
	auto const bounds = glm::vec2(m_bounds);
	ret.size = glm::uvec2(nSize * bounds);
	auto const pad = glm::uvec2(0.1f * bounds);
	ret.advance = ret.size.x + pad.x;
	ret.origin.x = -(int)pad.x / 2;
	ret.origin.y = (int)ret.size.y;
	if (auto it = m_glyphs.find(static_cast<u32>('X')); it != m_glyphs.end()) {
		auto const& X = it->second;
		ret.topLeft = X.topLeft + glm::ivec2(X.advance / 2, X.origin.y / 2);
	}
	return ret;
}

Glyph const& GlyphMap::glyph(u32 codepoint) const noexcept {
	if (auto it = m_glyphs.find(codepoint); it != m_glyphs.end()) { return it->second; }
	return blank();
}

Glyph const& GlyphMap::blank() const noexcept {
	if (m_blank.size.x == 0 && !m_glyphs.empty()) { m_blank = makeBlank(); }
	return m_blank;
}

bool GlyphMap::add(u32 codepoint, Glyph const& glyph) {
	if (glyph.valid()) {
		m_glyphs.insert_or_assign(codepoint, glyph);
		m_bounds = glyph.maxBounds(m_bounds);
		m_maxHeight = glyph.maxHeight(m_maxHeight);
		m_blank = makeBlank();
		return true;
	}
	return false;
}

void GlyphMap::remove(u32 codepoint) noexcept {
	m_glyphs.erase(codepoint);
	refresh();
}

void GlyphMap::refresh() {
	for (auto const& [_, glyph] : m_glyphs) {
		m_bounds = glyph.maxBounds(m_bounds);
		m_maxHeight = glyph.maxHeight(m_maxHeight);
		m_blank = makeBlank();
	}
}
} // namespace le::graphics
