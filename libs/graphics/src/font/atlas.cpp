#include <core/log_channel.hpp>
#include <core/utils/enumerate.hpp>
#include <graphics/font/atlas.hpp>
#include <algorithm>

namespace le::graphics {
namespace {
Glyph toGlyph(FontFace::Slot const& slot) noexcept { return {{}, slot.topLeft, slot.advance, slot.codepoint, slot.hasBitmap()}; }
} // namespace

FontAtlas::FontAtlas(not_null<VRAM*> const vram, CreateInfo const& info) : m_atlas(vram, info), m_face(vram->m_device), m_vram(vram) {}

bool FontAtlas::load(CommandBuffer const& cb, Span<std::byte const> const ttf, Size const size) noexcept {
	if (m_face.load(ttf, size)) {
		m_atlas.clear();
		m_glyphs.clear();
		auto slot = m_face.slot({});
		if (auto res = m_atlas.add({}, slot.pixmap, cb); res == TextureAtlas::Result::eOk || res == TextureAtlas::Result::eInvalidSize) {
			if (res == TextureAtlas::Result::eInvalidSize) { logW(LC_LibUser, "[Graphics] Zero glyph is missing texture"); }
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
			if (auto res = m_atlas.add(cp, slot.pixmap, cb); res == TextureAtlas::Result::eOk) {
				glyph.quad = m_atlas.get(cp);
			} else {
				if (res == TextureAtlas::Result::eSizeLocked) {
					logW(LC_LibUser, "[Graphics] FontAtlas size locked; cannot build new glyph [{} ({})]", static_cast<unsigned char>(cp), cp.value);
					return s_none;
				}
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
} // namespace le::graphics
