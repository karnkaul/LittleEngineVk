#include <core/log_channel.hpp>
#include <core/utils/enumerate.hpp>
#include <graphics/font/atlas.hpp>
#include <algorithm>

namespace le::graphics {
namespace {
Glyph toGlyph(FontFace::Slot const& slot) noexcept { return {{}, slot.topLeft, slot.advance, slot.codepoint, slot.hasBitmap()}; }
} // namespace

FontAtlas::FontAtlas(not_null<VRAM*> const vram, CreateInfo const& info) : m_atlas(vram, info), m_face(vram->m_device), m_info(info), m_vram(vram) {}

bool FontAtlas::load(CommandBuffer const& cb, Span<std::byte const> const ttf, Size const size) noexcept {
	if (m_face.load(ttf, size)) {
		m_atlas = TextureAtlas(m_vram, m_info);
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
} // namespace le::graphics
