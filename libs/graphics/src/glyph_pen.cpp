#include <graphics/common.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::graphics {
GlyphPen::GlyphPen(not_null<GlyphMap const*> glyphs, glm::uvec2 atlas, Size size, glm::vec3 pos, RGBA colour) noexcept : m_atlas(atlas), m_glyphs(glyphs) {
	reset(pos);
	this->colour(colour);
	this->size(size);
	m_state.yBound = (f32)m_glyphs->bounds().y * m_state.scale;
}

bool GlyphPen::generate(Geometry& out_geometry, Glyph const& glyph) const {
	if (glyph.valid()) {
		auto const c = m_state.colour.toVec4();
		auto head = m_state.head;
		head.y -= 0.5f * f32(m_glyphs->bounds().y) * m_state.scale;
		static constexpr auto normal = glm::vec3(0.0f);
		auto const offset = glm::vec3(invertY(glyph.origin), 0.0f);
		auto const tl = m_state.head - offset * m_state.scale;
		auto const atlas = glm::vec2(m_atlas);
		auto const gtl = glm::vec2(glyph.topLeft);
		auto const gsize = glm::vec2(glyph.size);
		auto const st = gtl / atlas;
		auto const uv = (gtl + gsize) / atlas;
		auto const size = gsize * m_state.scale;
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
	if (codepoint == '\n') {
		carriageReturn();
	} else if (auto const& gl = write(codepoint, out_geometry); gl.valid()) {
		m_state.advanced = m_state.scale * f32(gl.advance);
		m_state.head.x += m_state.advanced;
		m_state.lineExtent.x = std::max(m_state.lineExtent.x, std::abs(m_state.head.x - m_state.orgX));
		m_state.lineExtent.y = std::max(m_state.lineExtent.y, std::abs(f32(gl.size.y) * m_state.scale));
	}
	return m_state.head;
}

glm::vec3 GlyphPen::carriageReturn() noexcept {
	m_state.head.x = m_state.orgX;
	m_state.lineExtent = {};
	m_state.advanced = {};
	return m_state.head;
}

glm::vec3 GlyphPen::writeLine(std::string_view text, Geometry* out_geom, std::size_t* out_index) {
	std::size_t index = out_index ? *out_index : 0;
	for (char const ch : text) {
		if (ch == '\n') {
			g_log.log(lvl::warn, 0, "[{}] GlyphPen: Unexpected newline in string: [{}]", g_name, text);
			advance(' ', out_geom);
			return m_state.head;
		}
		advancePedal(index++, out_geom);
		advance(static_cast<u8>(ch), out_geom);
	}
	if (out_index) { *out_index = index; }
	return m_state.head;
}

GlyphPen::lines_t<std::string_view> GlyphPen::splitLines(std::string_view paragraph) noexcept {
	lines_t<std::string_view> ret;
	for (auto text = paragraph; !text.empty();) {
		std::string_view line = text;
		if (std::size_t const newline = line.find('\n'); newline < line.size()) {
			line = line.substr(0, newline); // cut till line
			text = text.substr(newline + 1);
		} else {
			text = {}; // entire text handled
		}
		ret.push_back(line);
		if (!ret.has_space()) { break; }
	}
	return ret;
}

GlyphPen::Para GlyphPen::writeLines(lines_t<std::string_view> const& lines, f32 const pad, std::size_t* out_index) {
	Para ret;
	// write lines and compute total height
	std::size_t index = out_index ? *out_index : 0;
	for (std::size_t i = 0; i < lines.size(); ++i) {
		Geometry geom;
		writeLine(lines[i], &geom, &index);
		advancePedal(index++, &geom);
		ret.lines.push_back({std::move(geom), extent()});
		ret.height += ret.lines.back().extent.y;
		// skip CR on last line (maintain head at end)
		if (i < lines.size() - 1) { carriageReturn(); }
	}
	f32 yOff = {};
	// raise each line by following lines * height
	for (std::size_t i = ret.lines.size(); i > 0; --i) {
		ret.lines[i - 1].geometry.offset({0.0f, yOff, 0.0f});
		yOff += ret.lines[i - 1].extent.y + pad * 2.0f;
	}
	return ret;
}

Geometry GlyphPen::Scribe::operator()(GlyphPen& out_pen, std::string_view const paragraph, glm::vec2 align, f32 pad) const {
	auto para = out_pen.writeLines(splitLines(paragraph), pad);
	Geometry ret;
	// apply offsets per line: line width, total height
	align -= 0.5f;
	for (auto& [geom, extent] : para.lines) {
		geom.offset({extent.x * align.x, para.height * align.y, 0.0f});
		ret.append(geom);
	}
	return ret;
}
} // namespace le::graphics
