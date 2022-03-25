#include <ktl/enumerate.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/graphics/font/font.hpp>
#include <levk/graphics/utils/instant_command.hpp>

namespace le::graphics {
namespace {
constexpr bool dummy(glm::vec2 const, std::size_t const) noexcept { return true; }

template <typename F = decltype(&dummy)>
glm::vec2 iterate(Font::Pen const& pen, f32 const scale, std::string_view line, F func = &dummy) {
	glm::vec2 ret{};
	for (auto const [ch, idx] : ktl::enumerate(line)) {
		if (ch == '\n' || ch == '\r') {
			logW(LC_LibUser, "[Graphics] Unexpected EOL in line [{}]", line);
			return ret;
		}
		auto const& gl = pen.glyph(Codepoint(static_cast<u32>(ch)));
		if (idx + 1 == line.size()) {
			ret.x += f32(gl.quad.extent.x) * scale;
		} else {
			ret.x += f32(gl.advance.x) * scale;
		}
		ret.y = std::max(ret.y, f32(gl.quad.extent.y) * scale);
		if (!func(ret, idx)) { return ret; }
	}
	ret.x = std::abs(ret.x);
	return ret;
}

glm::vec3 alignExtent(Font::Pen const& pen, f32 const scale, std::string_view const line, glm::vec2 pivot, glm::vec2 offset) {
	glm::vec3 ex{};
	pivot += 0.5f;
	if (line.empty()) {
		ex.y = f32(pen.glyph('A').quad.extent.y) * pivot.y * scale;
	} else {
		ex = glm::vec3((pen.lineExtent(line) + offset) * pivot, 0.0f);
	}
	return ex;
}
} // namespace

Font::Font(not_null<VRAM*> const vram, Info info) : m_vram(vram), m_info(std::move(info)) {
	if (m_info.name.empty()) { m_info.name = "(untitled)"; }
	auto [it, _] = m_atlases.emplace(m_info.height, FontAtlas(m_vram, m_info.atlas));
	load(it->second, m_info.height);
}

bool Font::add(Height height) {
	if (contains(height)) { return false; }
	FontAtlas at(m_vram, m_info.atlas);
	if (load(at, height)) {
		m_atlases.emplace(height, std::move(at));
		return true;
	}
	return false;
}

bool Font::remove(Height height) {
	if (height == m_info.height) {
		logW(LC_LibUser, "[Graphics] Cannot remove main size [{}] from Font [{}]", height, name());
		return false;
	}
	if (auto it = m_atlases.find(height); it != m_atlases.end()) {
		m_atlases.erase(it);
		return true;
	}
	return false;
}

std::vector<Font::Height> Font::sizes() const {
	std::vector<Height> ret;
	ret.reserve(sizeCount());
	for (auto const& [height, _] : m_atlases) { ret.push_back(Height{height}); }
	return ret;
}

FontAtlas const& Font::atlas(Height const height) const noexcept {
	auto it = m_atlases.find(height == Height{} ? m_info.height : height);
	EXPECT(it != m_atlases.end());
	return it->second;
}

f32 Font::scale(u32 height, Height const h) const noexcept { return f32(height) / f32(atlas(h).face().height()); }

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

bool Font::load(FontAtlas& out, Height height) {
	if (!out.load(m_info.ttf, height)) {
		logE(LC_EndUser, "[Graphics] Failed to load Font [{}]!", m_info.name);
		return false;
	}
	auto inst = InstantCommand(&m_vram->commandPool());
	for (Codepoint cp = m_info.preload.first; cp.value < m_info.preload.second.value; ++cp.value) { out.build(inst.m_scratch, inst.cb(), cp); }
	return true;
}

Font::Pen::Pen(not_null<Font*> font, PenInfo const& info)
	: m_cmd(&font->m_vram->commandPool()), m_info(info), m_head(info.origin), m_begin(info.origin), m_font(font) {
	if (m_info.scale <= 0.0f) { m_info.scale = 1.0f; }
	EXPECT(!m_info.customSize || m_font->contains(*m_info.customSize));
}

Glyph Font::Pen::glyph(Codepoint cp) const {
	auto& at = atlas();
	if (!at.contains(cp) && at.build(m_cmd.m_scratch, m_cmd.cb(), cp) != TextureAtlas::Result::eOk) { cp = {}; }
	return at.glyph(cp);
}

glm::vec2 Font::Pen::lineExtent(std::string_view const line) const { return iterate(*this, m_info.scale, line); }

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

void Font::Pen::align(std::string_view const line, glm::vec2 pivot) { m_head -= alignExtent(*this, m_info.scale, line, pivot, {}); }

glm::vec3 Font::Pen::writeLine(std::string_view line, Opt<glm::vec2 const> realign, Opt<std::size_t const> retIdx) {
	if (realign) { m_head -= alignExtent(*this, m_info.scale, line, *realign, {}); }
	auto write = [this](Codepoint const cp) {
		auto const& gl = glyph(cp);
		if (m_info.out_geometry) { m_font->write(*m_info.out_geometry, gl, m_head, m_info.scale); }
		advance(gl);
	};
	glm::vec3 idxPos = m_head;
	for (auto const [ch, idx] : ktl::enumerate(line)) {
		if (retIdx && *retIdx == idx) { idxPos = m_head; }
		Codepoint const cp(static_cast<u32>(ch));
		bool const newLine = ch == '\r' || ch == '\n';
		EXPECT(!newLine);
		if (newLine) { return retIdx ? idxPos : m_head; }
		if (ch == '\t') {
			Glyph const gl = glyph(Codepoint(static_cast<u32>(' ')));
			for (int i = 0; i < 4; ++i) { m_head += glm::vec3(gl.advance, 0.0f); }
		} else {
			write(cp);
		}
	}
	return !retIdx || *retIdx >= line.size() ? m_head : idxPos;
}

glm::vec3 Font::Pen::writeText(std::string_view text, Opt<glm::vec2 const> realign) {
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
	auto const h = f32(glyph('A').quad.extent.y);
	auto const dy = m_info.lineSpacing * h * m_info.scale;
	m_head.y -= dy;
	m_begin.y -= dy;
}

void Font::Pen::scale(f32 const s) noexcept {
	if (s > 0.0f) { m_info.scale = s; }
}

FontAtlas& Font::Pen::atlas() const {
	auto it = m_font->m_atlases.find(m_info.customSize ? *m_info.customSize : m_font->m_info.height);
	EXPECT(it != m_font->m_atlases.end());
	return it->second;
}
} // namespace le::graphics
