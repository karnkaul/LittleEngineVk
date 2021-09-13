#include <graphics/common.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::graphics {
GlyphPen::GlyphPen(not_null<GlyphMap const*> glyphs, glm::uvec2 atlas, Size size, glm::vec3 pos, RGBA colour) noexcept : m_atlas(atlas), m_glyphs(glyphs) {
	reset(pos);
	this->colour(colour);
	this->size(size);
}

bool GlyphPen::generate(Geometry& out_geometry, Glyph const& glyph) const {
	if (glyph.valid()) {
		auto const c = m_colour.toVec4();
		static constexpr auto normal = glm::vec3(0.0f);
		auto const offset = glm::vec3(invertY(glyph.origin), 0.0f);
		auto const tl = m_state.head - offset * m_scale;
		auto const atlas = glm::vec2(m_atlas);
		auto const gtl = glm::vec2(glyph.topLeft);
		auto const gsize = glm::vec2(glyph.size);
		auto const st = gtl / atlas;
		auto const uv = glyph.codepoint == codepoint_blank_v ? st : (gtl + gsize) / atlas;
		auto const size = gsize * m_scale;
		auto const vtl = out_geometry.addVertex({tl, c, normal, st});
		auto const vtr = out_geometry.addVertex({tl + glm::vec3(size.x, 0.0f, 0.0f), c, normal, {uv.x, st.y}});
		auto const vbr = out_geometry.addVertex({tl + glm::vec3(size.x, -size.y, 0.0f), c, normal, uv});
		auto const vbl = out_geometry.addVertex({tl + glm::vec3(0.0f, -size.y, 0.0f), c, normal, {st.x, uv.y}});
		std::array const indices = {vtl, vtr, vbr, vbr, vbl, vtl};
		out_geometry.addIndices(indices);
		return true;
	}
	return false;
}

Glyph const& GlyphPen::write(u32 codepoint, Geometry* out_geometry) const {
	ensure(codepoint != '\n');
	if (auto const& gl = glyph(codepoint); gl.valid()) {
		if (out_geometry) { generate(*out_geometry, gl); }
		return gl;
	}
	static constexpr Glyph blank{};
	return blank;
}

glm::vec3 GlyphPen::advance(u32 codepoint, Geometry* out_geometry) {
	if (auto const& gl = write(codepoint, out_geometry); gl.valid()) {
		m_state.advanced = m_scale * f32(gl.advance);								   // advance
		m_state.head.x += m_state.advanced;											   // head
		m_state.lineExtent.x = std::abs(m_state.head.x - m_state.orgX);				   // line x
		m_state.lineExtent.y = lineHeight();										   // line y
		m_state.totalExtent.x = std::max(m_state.totalExtent.x, m_state.lineExtent.x); // total x
		if (m_state.lineCount < 1) {
			m_state.lineCount = 1;						  // first line
			m_state.totalExtent.y = m_state.lineExtent.y; // set total y to line y
		}
	}
	return m_state.head;
}

glm::vec3 GlyphPen::carriageReturn() noexcept {
	m_state.head.x = m_state.orgX; // reset head
	m_state.lineExtent = {};	   // reset line extent
	m_state.advanced = {};		   // reset advanced
	return m_state.head;
}

glm::vec3 GlyphPen::lineFeed(f32 nPad) noexcept {
	carriageReturn();
	f32 const offset = m_glyphs->lineHeight(m_scale) * (1.0f + nPad); // incorporate line pad into offset
	m_state.head.y -= offset;										  // apply offset to head
	m_state.totalExtent.y += offset;								  // add offset to total extent
	++m_state.lineCount;											  // increment line count
	return m_state.head;
}

glm::vec3 GlyphPen::writeLine(std::string_view text, Geometry* out_geom, std::size_t const* index) {
	glm::vec3 ret = head();
	for (std::size_t idx = 0; idx < text.size(); ++idx) {
		if (index && *index == idx) { ret = head(); } // set head to this character
		char const ch = text[idx];
		if (ch == '\n') {
			g_log.log(lvl::warn, 0, "[{}] GlyphPen2: Unexpected newline in string: [{}]", g_name, text);
			advance(' ', out_geom);
			return ret;
		}
		advance(static_cast<u32>(ch), out_geom); // advance for next glyph
	}
	if (!index || *index >= text.size()) { ret = head(); } // index past last glyph
	return ret;
}

glm::vec3 GlyphPen::offsetY(std::size_t lineCount, f32 nPad) noexcept {
	// first line is already above baseline, no need to offset that case
	// multiple lines: offset head by (lines - 1) * (line_height + line_pad)
	m_state.head.y += f32(lineCount == 0 ? 0 : lineCount - 1) * m_glyphs->lineHeight(m_scale) * (1.0f + nPad);
	return m_state.head;
}

GlyphPen::lines_t<std::string_view> GlyphPen::splitLines(std::string_view paragraph) noexcept {
	if (paragraph.empty()) { return {}; }
	lines_t<std::string_view> ret;
	bool lastBlank{}; // if last line is only \n
	for (auto text = paragraph; !text.empty();) {
		std::string_view line = text;
		if (std::size_t const newline = line.find('\n'); newline < line.size()) {
			line = line.substr(0, newline); // cut till line
			text = text.substr(newline + 1);
			lastBlank = text.empty(); // nothing after last \n
		} else {
			text = {}; // entire text handled
		}
		ret.push_back(line);
		if (!ret.has_space()) { break; }
	}
	if (lastBlank) { ret.push_back({}); }
	return ret;
}

GlyphPen::LineIndex GlyphPen::lineIndex(GlyphPen::lines_t<std::string_view> const& lines, std::size_t index) noexcept {
	if (lines.empty()) { return {}; }
	LineIndex ret;
	ret.line = lines.size() - 1;		 // last line
	ret.character = lines.back().size(); // past last character
	bool set{};
	for (std::size_t lidx = 0; lidx < lines.size(); ++lidx) {
		std::string_view const line = lines[lidx];
		std::size_t const length = line.size() + (lidx == lines.size() - 1 ? 0 : 1); // add \n except on last line
		if (!set) {
			if (index >= length) {
				index -= length; // subtract this line's length (include \n)
			} else {
				ret.character = index; // char is in this line
				ret.line = lidx;	   // set line index
				set = true;
			}
		}
	}
	return ret;
}

glm::vec3 GlyphPen::alignOffset(glm::vec2 lineExtent, glm::vec2 nAlign) const noexcept {
	nAlign = -nAlign - 0.5f;
	f32 const y = m_state.totalExtent.y == 0.0f ? lineExtent.y : m_state.totalExtent.y; // use line height if total height is 0
	return {lineExtent.x * nAlign.x, y * nAlign.y, 0.0f};
}

GlyphPen::Para GlyphPen::writeLines(lines_t<std::string_view> const& lines, LineInfo const& info) {
	Para ret;
	offsetY(lines.size(), info.nLinePad);																  // move head up to write these many lines
	LineIndex const li = lineIndex(lines, info.returnIndex ? *info.returnIndex : std::string_view::npos); // get desired line index
	ret.head = head();																					  // set return head to start
	for (std::size_t i = 0; i < lines.size(); ++i) {
		Geometry gm;
		if (li.line == i) {
			ret.head = writeLine(lines[i], info.produceGeometry ? &gm : nullptr, &li.character); // set return head to glyph in this line
		} else {
			writeLine(lines[i], info.produceGeometry ? &gm : nullptr, &li.character); // only write lines, don't modify return head
		}
		ret.lines.push_back({std::move(gm), {m_state.lineExtent.x, m_state.lineExtent.y}});
		if (i < lines.size() - 1) { lineFeed(info.nLinePad); } // skip LF on last line (maintain head at end)
	}
	return ret;
}

void GlyphPen::alignLines(lines_t<Line>& out_lines, glm::vec2 nAlign, Geometry* out) const {
	// apply offsets per line: line width, total height
	for (auto& [geom, lext] : out_lines) {
		geom.offset(alignOffset(lext, nAlign));
		if (out) { out->append(geom); }
	}
}

Geometry GlyphPen::generate(std::string_view const paragraph, f32 nLinePad, glm::vec2 nAlign) {
	Geometry ret;
	LineInfo info;
	info.nLinePad = nLinePad;
	auto para = writeLines(splitLines(paragraph), info);
	alignLines(para.lines, nAlign, &ret);
	return ret;
}
} // namespace le::graphics
