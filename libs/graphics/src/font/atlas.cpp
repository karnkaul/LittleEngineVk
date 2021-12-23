#include <core/log_channel.hpp>
#include <core/utils/enumerate.hpp>
#include <graphics/font/atlas.hpp>
#include <algorithm>

namespace le::graphics {
namespace {
Glyph toGlyph(FontFace::Slot const& slot) noexcept { return {{}, slot.topLeft, slot.advance, slot.codepoint, slot.hasBitmap()}; }
} // namespace

FontAtlas::FontAtlas(not_null<VRAM*> const vram, CreateInfo const& info)
	: m_atlas(vram, info.atlas), m_face(vram->m_device), m_atlasInfo(info.atlas), m_vram(vram) {}

bool FontAtlas::load(CommandBuffer const& cb, Span<std::byte const> const ttf, Size const size) noexcept {
	if (m_face.load(ttf, size)) {
		m_atlas = TextureAtlas(m_vram, m_atlasInfo);
		m_glyphs.clear();
		auto slot = m_face.slot({});
		if (m_atlas.add({}, slot.pixmap, cb)) {
			m_glyphs.emplace(Codepoint{}, toGlyph(slot));
		} else {
			logW(LC_LibUser, "[Graphics] Failed to get zero glyph");
		}
		return true;
	}
	return false;
}

Glyph const& FontAtlas::build(CommandBuffer const& cb, Codepoint const cp, bool const rebuild) {
	if (!rebuild || cp == Codepoint{}) {
		if (auto it = m_glyphs.find(cp); it != m_glyphs.end()) {
			auto& ret = it->second;
			if (m_atlas.contains(cp)) { ret.quad = m_atlas.get(cp); }
			return ret;
		}
	}
	auto slot = m_face.slot(cp);
	static Glyph const s_none;
	EXPECT(cp != Codepoint{});
	if (slot.codepoint == cp) {
		auto glyph = toGlyph(slot);
		if (glyph.textured) {
			if (m_atlas.add(cp, slot.pixmap, cb)) {
				glyph.quad = m_atlas.get(cp);
			} else {
				logW(LC_LibUser, "[Graphics] Failed to add glyph [{} ({})] to texture atlas", static_cast<unsigned char>(cp), cp.value);
				glyph.quad = m_atlas.get({});
				slot = m_face.slot({});
				slot.codepoint = cp;
			}
		}
		auto [it, _] = m_glyphs.insert_or_assign(cp, glyph);
		return it->second;
	}
	return s_none;
}

Extent2D FontAtlas::extent(CommandBuffer const& cb, std::string_view line) {
	glm::ivec2 ret{};
	for (auto const [ch, idx] : le::utils::enumerate(line)) {
		if (ch == '\n' || ch == '\r') {
			logW(LC_LibUser, "[Graphics] Unexpected EOL in line [{}]", line);
			return ret;
		}
		Codepoint const cp{u32(ch)};
		bool const valid = Codepoint::Validate<>{}(cp);
		auto gl = valid ? build(cb, cp) : build(cb, {});
		if (valid && gl.codepoint != cp) { gl = build(cb, {}); }
		if (idx + 1 == line.size()) {
			ret += gl.quad.extent.x;
		} else {
			ret.x += gl.advance.x;
		}
		ret.y = std::max(ret.y, int(gl.quad.extent.y));
	}
	ret.x = std::abs(ret.x);
	return ret;
}

bool FontAtlas::write(Geometry& out, glm::vec3 const origin, Glyph const& glyph) const {
	if (glyph.textured) {
		GeomInfo gi;
		auto const hs = glm::vec2(glyph.quad.extent) * 0.5f;
		gi.origin = origin + glm::vec3(hs.x, -hs.y, 0.0f) + glm::vec3(glyph.topLeft, 0.0f);
		out.append(makeQuad(glyph.quad.extent, gi, glyph.quad.uv));
		return true;
	}
	return false;
}
} // namespace le::graphics
