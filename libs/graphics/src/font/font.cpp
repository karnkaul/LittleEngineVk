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

Font::Pen::Pen(not_null<Font*> font, PenInfo const& info)
	: m_cmd(&font->m_vram->commandPool()), m_info(info), m_head(info.origin), m_begin(info.origin), m_font(font) {
	if (m_info.scale <= 0.0f) { m_info.scale = 1.0f; }
}

Glyph const& Font::Pen::glyph(Codepoint cp) const {
	auto ret = &m_font->m_atlas.build(m_cmd.cb(), cp);
	if (ret->codepoint != cp) { ret = &m_font->m_atlas.build(m_cmd.cb(), {}); }
	return *ret;
}

glm::vec2 Font::Pen::lineExtent(std::string_view const line) const {
	glm::vec2 ret{};
	for (auto const [ch, idx] : le::utils::enumerate(line)) {
		if (ch == '\n' || ch == '\r') {
			logW(LC_LibUser, "[Graphics] Unexpected EOL in line [{}]", line);
			return ret;
		}
		auto const& gl = glyph(Codepoint(static_cast<u32>(ch)));
		if (idx + 1 == line.size()) {
			ret.x += f32(gl.quad.extent.x) * m_info.scale;
		} else {
			ret.x += f32(gl.advance.x) * m_info.scale;
		}
		ret.y = std::max(ret.y, f32(gl.quad.extent.y) * m_info.scale);
	}
	ret.x = std::abs(ret.x);
	return ret;
}

glm::vec2 Font::Pen::textExtent(std::string_view text) const {
	glm::vec2 ret{};
	auto const update = [&ret, this](std::string_view line) {
		auto const le = lineExtent(line);
		ret.x = std::max(ret.x, le.x);
		ret.y += le.y;
	};
	auto remain = text;
	auto idx = remain.find('\n');
	while (idx != std::string_view::npos) {
		auto const line = remain.substr(0, idx);
		remain = remain.substr(idx + 1);
		update(line);
		idx = remain.find('\n');
	}
	if (!remain.empty()) { update(remain); }
	return ret;
}

void Font::Pen::align(std::string_view const line, glm::vec2 pivot) {
	glm::vec3 ex{};
	pivot += 0.5f;
	if (line.empty()) {
		ex.y = f32(glyph('A').quad.extent.y) * pivot.y * m_info.scale;
	} else {
		ex = glm::vec3(lineExtent(line) * pivot, 0.0f);
	}
	m_head -= ex;
}

glm::vec3 Font::Pen::writeLine(std::string_view const line, std::optional<glm::vec2> const realign, std::size_t const* retIdx) {
	if (realign) { align(line, *realign); }
	glm::vec3 idxPos = m_head;
	for (auto const [ch, idx] : le::utils::enumerate(line)) {
		if (retIdx && *retIdx == idx) { idxPos = m_head; }
		Codepoint const cp(static_cast<u32>(ch));
		bool const newLine = ch == '\r' || ch == '\n';
		EXPECT(!newLine);
		if (newLine) { return retIdx ? idxPos : m_head; }
		if (ch == '\t') {
			Glyph const& glyph = m_font->m_atlas.build(m_cmd.cb(), Codepoint(static_cast<u32>(' ')));
			for (int i = 0; i < 4; ++i) { m_head += glm::vec3(glyph.advance, 0.0f); }
		} else {
			auto const& gl = glyph(cp);
			if (m_info.out_geometry) { m_font->write(*m_info.out_geometry, gl, m_head, m_info.scale); }
			advance(gl);
		}
	}
	return !retIdx || *retIdx >= line.size() ? m_head : idxPos;
}

glm::vec3 Font::Pen::writeText(std::string_view text, std::optional<glm::vec2> const realign) {
	auto remain = text;
	auto idx = remain.find('\n');
	while (idx != std::string_view::npos) {
		auto const line = remain.substr(0, idx);
		remain = remain.substr(idx + 1);
		writeLine(line, realign);
		lineFeed();
		idx = remain.find('\n');
	}
	if (!remain.empty()) { writeLine(remain, realign); }
	return m_head;
}

void Font::Pen::lineFeed() noexcept {
	m_head = m_begin;
	auto const h = f32(m_font->face().size().height);
	auto const dy = (m_info.lineSpacing - 0.5f) * h;
	m_head.y -= dy;
	m_begin.y -= dy;
}

void Font::Pen::scale(f32 const s) noexcept {
	if (s > 0.0f) { m_info.scale = s; }
}
} // namespace le::graphics
