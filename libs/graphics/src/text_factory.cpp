#include <graphics/text_factory.hpp>

namespace le::graphics {
Geometry TextFactory::generate(Span<Glyph const> glyphs, std::string_view text, glm::ivec2 texSize, std::optional<Layout> layout) const noexcept {
	if (text.empty()) { return {}; }
	if (!layout) { layout = this->layout(glyphs, text, size, nYPad); }
	glm::vec2 const realTopLeft = pos;
	glm::vec2 textTL = realTopLeft;
	std::size_t nextLineIdx = 0;
	s32 yIdx = 0;
	f32 xPos = 0.0f;
	f32 lineWidth = 0.0f;
	glm::vec2 const textTLoffset = {align.x - 0.5f, align.y + 0.5f};
	auto updateTextTL = [&]() {
		lineWidth = 0.0f;
		f32 maxOffsetY = 0.0f;
		for (; nextLineIdx < text.size(); ++nextLineIdx) {
			auto const ch = text[nextLineIdx];
			if (ch == '\n') {
				break;
			} else {
				Glyph const& glyph = glyphs[(std::size_t)ch];
				lineWidth += (f32)glyph.xAdv;
				maxOffsetY = std::max(maxOffsetY, (f32)glyph.offset.y);
			}
		}
		maxOffsetY *= layout->scale;
		f32 const offsetY = layout->lineHeight - maxOffsetY;
		lineWidth *= layout->scale;
		++nextLineIdx;
		xPos = 0.0f;
		textTL = realTopLeft + textTLoffset * glm::vec2(lineWidth, layout->textHeight + offsetY);
		textTL.y -= (layout->lineHeight + ((f32)yIdx * (layout->lineHeight + layout->linePad)));
	};
	updateTextTL();

	Geometry ret;
	u32 quadCount = (u32)text.length();
	ret.reserve(4 * quadCount, 6 * quadCount);
	auto const normal = glm::vec3(0.0f);
	for (auto const ch : text) {
		if (ch == '\n') {
			++yIdx;
			updateTextTL();
			continue;
		}
		std::size_t const idx = (std::size_t)ch;
		if (idx >= glyphs.size()) { continue; }
		glm::vec3 const c(colour.toVec4());
		auto const& glyph = glyphs[idx];
		auto const offset = glm::vec3(xPos - (f32)glyph.offset.x * layout->scale, (f32)glyph.offset.y * layout->scale, 0.0f);
		auto const tl = glm::vec3(textTL.x, textTL.y, pos.z) + offset;
		auto const s = (f32)glyph.st.x / (f32)texSize.x;
		auto const t = (f32)glyph.st.y / (f32)texSize.y;
		auto const u = s + (f32)glyph.uv.x / (f32)texSize.x;
		auto const v = t + (f32)glyph.uv.y / (f32)texSize.y;
		glm::vec2 const cell = {(f32)glyph.cell.x * layout->scale, (f32)glyph.cell.y * layout->scale};
		auto const v0 = ret.addVertex({tl, c, normal, glm::vec2(s, t)});
		auto const v1 = ret.addVertex({tl + glm::vec3(cell.x, 0.0f, 0.0f), c, normal, glm::vec2(u, t)});
		auto const v2 = ret.addVertex({tl + glm::vec3(cell.x, -cell.y, 0.0f), c, normal, glm::vec2(u, v)});
		auto const v3 = ret.addVertex({tl + glm::vec3(0.0f, -cell.y, 0.0f), c, normal, glm::vec2(s, v)});
		std::array const indices = {v0, v1, v2, v2, v3, v0};
		ret.addIndices(indices);
		xPos += ((f32)glyph.xAdv * layout->scale);
	}
	return ret;
}

glm::ivec2 TextFactory::glyphBounds(Span<Glyph const> glyphs, std::string_view text) const noexcept {
	glm::ivec2 ret = {};
	if (text.empty()) {
		for (auto const& glyph : glyphs) {
			ret.x = std::max(ret.x, glyph.cell.x);
			ret.y = std::max(ret.y, glyph.cell.y);
		}
	} else {
		for (char c : text) {
			std::size_t const idx = (std::size_t)c;
			if (idx < glyphs.size()) {
				ret.x = std::max(ret.x, glyphs[idx].cell.x);
				ret.y = std::max(ret.y, glyphs[idx].cell.y);
			}
		}
	}
	return ret;
}

TextFactory::Layout TextFactory::layout(Span<Glyph const> glyphs, std::string_view text, Size size, f32 nPadY) const noexcept {
	Layout ret;
	ret.maxBounds = glyphBounds(glyphs, text);
	ret.lineCount = 1;
	for (std::size_t idx = 0; idx < text.size(); ++idx) {
		if (text[idx] == '\n') { ++ret.lineCount; }
	}
	size.visit(ktl::overloaded{[&](u32 u) { ret.scale = (f32)u / (f32)glyphBounds(glyphs).y; }, [&ret](f32 f) { ret.scale = f; }});
	ret.lineHeight = (f32)ret.maxBounds.y * ret.scale;
	ret.linePad = nPadY * ret.lineHeight;
	ret.textHeight = (f32)ret.lineHeight * ((f32)ret.lineCount + nPadY * f32(ret.lineCount - 1));
	return ret;
}
} // namespace le::graphics
