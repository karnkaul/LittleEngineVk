#include <graphics/common.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::graphics {
BitmapGlyphPen::BitmapGlyphPen(not_null<BitmapGlyphArray const*> glyphs, glm::uvec2 atlas, Size size, glm::vec3 pos, RGBA colour) noexcept
	: m_atlas(atlas), m_glyphs(glyphs) {
	reset(pos);
	reset(colour);
	resize(size);
}

bool BitmapGlyphPen::generate(Geometry& out_geometry, BitmapGlyph const& glyph) const {
	if (glyph.valid()) {
		auto const c = m_state.colour.toVec4();
		auto head = m_state.head;
		head.y -= 0.5f * f32(m_glyphs->bounds().y) * m_state.scale;
		static constexpr auto normal = glm::vec3(0.0f);
		glm::vec3 const offset = {(f32)glyph.offset.x, -(f32)glyph.offset.y, 0.0f};
		auto const tl = m_state.head - offset * m_state.scale;
		auto const s = (f32)glyph.st.x / (f32)m_atlas.x;
		auto const t = (f32)glyph.st.y / (f32)m_atlas.y;
		auto const u = s + (f32)glyph.uv.x / (f32)m_atlas.x;
		auto const v = t + (f32)glyph.uv.y / (f32)m_atlas.y;
		glm::vec2 const cell = glm::vec2((f32)glyph.cell.x, (f32)glyph.cell.y) * m_state.scale;
		auto const v0 = out_geometry.addVertex({tl, c, normal, glm::vec2(s, t)});
		auto const v1 = out_geometry.addVertex({tl + glm::vec3(cell.x, 0.0f, 0.0f), c, normal, glm::vec2(u, t)});
		auto const v2 = out_geometry.addVertex({tl + glm::vec3(cell.x, -cell.y, 0.0f), c, normal, glm::vec2(u, v)});
		auto const v3 = out_geometry.addVertex({tl + glm::vec3(0.0f, -cell.y, 0.0f), c, normal, glm::vec2(s, v)});
		std::array const indices = {v0, v1, v2, v2, v3, v0};
		out_geometry.addIndices(indices);
		return true;
	}
	return false;
}

glm::vec3 BitmapGlyphPen::advance(u8 ch, Geometry* out_geometry) {
	if (ch == '\n') {
		carriageReturn();
	} else if (auto const& gl = glyph(ch); gl.valid()) {
		if (out_geometry) { generate(*out_geometry, gl); }
		m_state.head.x += (f32)gl.xAdv * m_state.scale;
		m_state.lineExtent.x = std::max(m_state.lineExtent.x, std::abs(m_state.head.x - m_state.orgX));
		m_state.lineExtent.y = std::max(m_state.lineExtent.y, std::abs(f32(gl.cell.y) * m_state.scale));
	}
	return m_state.head;
}

glm::vec3 BitmapGlyphPen::carriageReturn(bool nextLine) noexcept {
	m_state.head.x = m_state.orgX;
	if (nextLine) {
		f32 const bump = (f32)m_glyphs->bounds().y * m_state.scale + m_linePad;
		m_state.head.y -= bump;
		m_state.firstChar = bump;
		m_state.lineExtent = {};
		++m_state.lineCount;
	}
	return m_state.head;
}

glm::vec3 BitmapGlyphPen::writeLine(std::string_view text, Geometry* out) {
	for (char const ch : text) {
		if (ch == '\n') {
			g_log.log(lvl::warning, 0, "[{}] BitmapGlyphPen: Unexpected newline in string: [{}]", g_name, text);
			advance(' ', out);
			return m_state.head;
		}
		advance(static_cast<u8>(ch), out);
	}
	return m_state.head;
}

glm::vec3 BitmapGlyphPen::write(std::string_view text, glm::vec2 align, Geometry* out) {
	align -= 0.5f;
	glm::vec2 extent{}; // final extent will be max width x total height
	ktl::fixed_vector<Geometry, 16> lines;
	for (; !text.empty(); carriageReturn()) {
		std::string_view line = text;
		if (std::size_t const newline = line.find('\n'); newline < line.size()) {
			line = line.substr(0, newline); // cut till line
			text = text.substr(newline + 1);
		} else {
			text = {}; // entire text handled
		}
		Geometry geom;
		writeLine(line, out ? &geom : nullptr);
		if (out) {
			lines.push_back(std::move(geom));
			lines.back().offset({m_state.lineExtent.x * align.x, 0.0f, 0.0f}); // offset line's x by width * align.x
		}
		extent = {std::max(extent.x, m_state.lineExtent.x), m_state.lineExtent.y}; // update total extent
	}
	if (out && !lines.empty()) {
		for (Geometry& geom : lines) {
			geom.offset({0.0f, extent.y * align.y, 0.0f}); // offset line's y by total extent.y * align.y
			append(*out, geom);
		}
	}
	return m_state.head;
}

Geometry BitmapGlyphPen::generate(std::string_view text, glm::vec2 align) {
	Geometry ret;
	write(text, align, &ret);
	return ret;
}
} // namespace le::graphics
