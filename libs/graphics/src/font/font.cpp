#include <core/log_channel.hpp>
#include <core/utils/enumerate.hpp>
#include <graphics/font/font.hpp>
#include <graphics/utils/instant_command.hpp>

namespace le::graphics {
Font::Font(not_null<VRAM*> const vram, Info info) : m_vram(vram), m_atlas(vram, info.atlas), m_name(std::move(info.name)) {
	if (m_name.empty()) { m_name = "(untitled)"; }
	load(std::move(info));
}

bool Font::load(Info info) {
	auto inst = InstantCommand(&m_vram->commandPool());
	if (!m_atlas.load(inst.cb(), info.ttf, info.size)) {
		logE(LC_EndUser, "[Graphics] Failed to load Font [{}]!", m_name);
		return false;
	}
	for (Codepoint cp = info.preload.first; cp.value < info.preload.second.value; ++cp.value) { m_atlas.build(inst.cb(), cp); }
	if (!info.name.empty()) { m_name = std::move(info.name); }
	return true;
}

f32 Font::scale(u32 height) const noexcept { return f32(height) / f32(m_atlas.face().size().height); }

bool Font::write(Geometry& out, Glyph const& glyph, glm::vec3 const m_head, f32 const scale) const {
	if (glyph.textured && scale > 0.0f) {
		GeomInfo gi;
		auto const hs = glm::vec2(glyph.quad.extent) * 0.5f * scale;
		gi.origin = m_head + glm::vec3(hs.x, -hs.y, 0.0f) + glm::vec3(glyph.topLeft, 0.0f) * scale;
		out.append(makeQuad(glm::vec2(glyph.quad.extent) * scale, gi, glyph.quad.uv));
		return true;
	}
	return false;
}

Font::Pen::Pen(not_null<Font*> font, Geometry* const geom, glm::vec3 const head, f32 const scale)
	: m_cmd(&font->m_vram->commandPool()), m_head(head), m_scale(scale), m_geom(geom), m_font(font) {
	if (m_scale <= 0.0f) { m_scale = 1.0f; }
}

Glyph const& Font::Pen::glyph(Codepoint cp) const {
	auto ret = &m_font->m_atlas.build(m_cmd.cb(), cp);
	if (ret->codepoint != cp) { ret = &m_font->m_atlas.build(m_cmd.cb(), {}); }
	return *ret;
}

glm::vec2 Font::Pen::extent(std::string_view const line) const {
	glm::vec2 ret{};
	for (auto const [ch, idx] : le::utils::enumerate(line)) {
		if (ch == '\n' || ch == '\r') {
			logW(LC_LibUser, "[Graphics] Unexpected EOL in line [{}]", line);
			return ret;
		}
		auto const& gl = glyph(Codepoint(static_cast<u32>(ch)));
		if (idx + 1 == line.size()) {
			ret.x += f32(gl.quad.extent.x) * m_scale;
		} else {
			ret.x += f32(gl.advance.x) * m_scale;
		}
		ret.y = std::max(ret.y, f32(gl.quad.extent.y) * m_scale);
	}
	ret.x = std::abs(ret.x);
	return ret;
}

void Font::Pen::align(std::string_view const line, glm::vec2 const pivot) { m_head -= glm::vec3(extent(line) * (pivot + 0.5f), 0.0f); }

glm::vec3 Font::Pen::write(std::string_view const line, std::optional<glm::vec2> const realign, std::size_t const* retIdx) {
	if (realign) { align(line, *realign); }
	glm::vec3 idxPos = m_head;
	for (auto const [ch, idx] : le::utils::enumerate(line)) {
		if (retIdx && *retIdx == idx) { idxPos = m_head; }
		EXPECT(ch != '\n' && ch != '\r');
		Codepoint const cp(static_cast<u32>(ch));
		if (ch == '\n' || ch == '\r') {
			logW("[Graphics] Unexpected newline (Font: {})", m_font->m_name);
			return retIdx ? idxPos : m_head;
		}
		if (ch == '\t') {
			Glyph const& glyph = m_font->m_atlas.build(m_cmd.cb(), Codepoint(static_cast<u32>(' ')));
			for (int i = 0; i < 4; ++i) { m_head += glm::vec3(glyph.advance, 0.0f); }
			continue;
		}
		auto const& gl = glyph(cp);
		if (m_geom) { m_font->write(*m_geom, gl, m_head, m_scale); }
		advance(gl);
	}
	return !retIdx || *retIdx >= line.size() ? m_head : idxPos;
}

void Font::Pen::scale(f32 const s) noexcept {
	if (s > 0.0f) { m_scale = s; }
}
} // namespace le::graphics
