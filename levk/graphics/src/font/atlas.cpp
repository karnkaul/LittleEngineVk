#include <core/log_channel.hpp>
#include <core/utils/enumerate.hpp>
#include <levk/graphics/font/atlas.hpp>
#include <algorithm>

namespace le::graphics {
namespace {
Glyph toGlyph(FontFace::Slot const& slot) noexcept { return {{}, slot.topLeft, slot.advance, slot.codepoint, slot.hasBitmap()}; }
} // namespace

FontAtlas::FontAtlas(not_null<VRAM*> const vram, CreateInfo const& info) : m_atlas(vram, info), m_face(vram->m_device), m_vram(vram) {}

bool FontAtlas::load(Span<std::byte const> const ttf, Height const height) noexcept {
	if (m_face.load(ttf, height)) {
		m_atlas.clear();
		m_glyphs.clear();
		return true;
	}
	return false;
}

Glyph FontAtlas::glyph(Codepoint cp) const noexcept {
	auto it = m_glyphs.find(cp);
	if (it == m_glyphs.end()) { it = m_glyphs.find({}); }
	if (it != m_glyphs.end()) {
		auto ret = it->second;
		if (m_atlas.contains(cp)) { ret.quad = m_atlas.get(cp); }
		return ret;
	}
	return {};
}

FontAtlas::Result FontAtlas::build(CommandBuffer const& cb, Codepoint const cp, bool const rebuild) {
	Result ret;
	if (!rebuild || cp == Codepoint{}) {
		if (m_glyphs.contains(cp)) {
			ret.outcome = Outcome::eOk;
			return ret;
		}
	}
	auto slot = m_face.slot(cp);
	if (slot.codepoint == cp) {
		auto glyph = toGlyph(slot);
		if (glyph.textured) {
			if (auto res = m_atlas.add(cp, slot.pixmap, cb); res.outcome == Outcome::eOk) {
				glyph.quad = m_atlas.get(cp);
				ret = std::move(res);
			} else {
				if (res.outcome == Outcome::eSizeLocked) {
					logW(LC_LibUser, "[Graphics] FontAtlas size locked; cannot build new glyph [{} ({})]", static_cast<unsigned char>(cp), cp.value);
					return res;
				}
				logW(LC_LibUser, "[Graphics] Failed to add glyph [{} ({})] to texture atlas", static_cast<unsigned char>(cp), cp.value);
				glyph.quad = m_atlas.get({});
				slot = m_face.slot({});
				slot.codepoint = cp;
				ret = std::move(res);
			}
		}
		m_glyphs.insert_or_assign(cp, glyph);
	}
	return ret;
}
} // namespace le::graphics
