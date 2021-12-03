#include <core/services.hpp>
#include <engine/input/state.hpp>
#include <engine/input/text_cursor.hpp>
#include <engine/render/bitmap_font.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::input {
TextCursor::TextCursor(not_null<BitmapFont const*> font, Flags flags)
	: m_flags(flags), m_primitive(Services::get<graphics::VRAM>(), graphics::MeshPrimitive::Type::eDynamic), m_font(font) {
	refresh();
}

MeshView TextCursor::mesh() const noexcept {
	if (m_drawCursor) {
		m_material.Tf = m_gen.colour;
		m_material.d = m_alpha;
		return MeshObj{&m_primitive, &m_material};
	}
	return {};
}

void TextCursor::backspace(graphics::Geometry* out) {
	if (m_text.empty() || m_index == 0) {
		refresh(out);
		return;
	}
	if (m_index - 1 >= m_text.size()) {
		m_text.pop_back();
		if (m_index > 0) { --m_index; }
	} else {
		m_text.erase(m_index-- - 1, 1);
	}
	refresh();
}

void TextCursor::deleteFront(graphics::Geometry* out) {
	if (!m_text.empty() && m_index < m_text.size()) { m_text.erase(m_index, 1); }
	refresh(out);
}

void TextCursor::insert(char ch, graphics::Geometry* out) {
	if (m_index >= m_text.size()) {
		m_text += ch;
		if (m_index < npos) { ++m_index; }
	} else {
		m_text.insert(m_index++, 1, ch);
	}
	refresh(out);
}

bool TextCursor::update(State const& state, graphics::Geometry* out) {
	if (state.pressed(Key::eEscape)) { setActive(false); }
	if (!m_flags.test(Flag::eActive)) {
		// hide cursor and ignore input
		m_drawCursor = false;
		return false;
	}
	bool refr = {};
	if (state.pressOrRepeat(Key::eBackspace)) {
		backspace(nullptr);
		refr = true;
	}
	if (state.pressOrRepeat(Key::eDelete)) {
		deleteFront(nullptr);
		refr = true;
	}
	if (!m_flags.test(Flag::eNoNewLine) && state.pressOrRepeat(Key::eEnter)) {
		insert('\n', nullptr);
		refr = true;
	}
	if (state.pressOrRepeat(Key::eLeft)) {
		if (m_index >= m_text.size()) {
			// reset to size - 1 if beyond it
			m_index = m_text.size() - 1;
		} else if (m_index > 0) {
			// decrement if > 0
			--m_index;
		}
		refr = true;
	}
	if (state.pressOrRepeat(Key::eRight)) {
		// increment if less than size
		if (m_index <= m_text.size()) { ++m_index; }
		refr = true;
	}
	for (u32 const codepoint : state.codepoints) {
		if (Codepoint::Validate{}(codepoint)) {
			insert(static_cast<char>(codepoint), nullptr);
			refr = true;
		}
	}
	if (!m_flags.test(Flag::eNoAutoBlink)) {
		if (refr) {
			m_lastBlink = time::now();
			m_drawCursor = true;
		}
		if (m_lastBlink == time::Point()) { m_lastBlink = time::now(); }
		auto lastBlink = m_lastBlink;
		Time_s const elapsed = time::diffExchg(lastBlink);
		if (elapsed >= m_blinkRate) {
			m_drawCursor = !m_drawCursor;
			m_lastBlink = lastBlink;
		}
	}
	if (refr) {
		refresh(out);
		return true;
	}
	return false;
}

void TextCursor::refresh(graphics::Geometry* out) {
	using Pen = graphics::GlyphPen;
	Pen pen(&m_font->glyphs(), m_font->atlasSize(), m_gen.size, m_gen.position, m_gen.colour);
	auto const lines = pen.splitLines(m_text);
	auto const lineIndex = pen.lineIndex(lines, m_index);
	Pen::LineInfo const info{.nLinePad = m_gen.nLinePad, .produceGeometry = out != nullptr, .returnIndex = &m_index};
	auto para = pen.writeLines(lines, info);
	pen.alignLines(para.lines, m_gen.align, out);
	glm::vec2 const extent = para.lines.size() <= lineIndex.line ? glm::vec2(0.0f, pen.lineHeight()) : para.lines[lineIndex.line].extent;
	auto const size = pen.scale() * m_size * glm::vec2(pen.glyphs().bounds());
	m_position = para.head + pen.alignOffset(extent, m_gen.align);
	m_position.y += 0.3f * size.y;
	graphics::GeomInfo const gi{.origin = m_position};
	m_primitive.construct(graphics::makeQuad(size, gi));
}

graphics::Geometry TextCursor::generateText() {
	graphics::Geometry ret;
	refresh(&ret);
	return ret;
}

void TextCursor::setActive(bool active) noexcept {
	m_drawCursor = active;
	m_flags.assign(Flag::eActive, active);
}

void TextCursor::index(std::size_t index) {
	if (index != m_index) {
		m_index = index;
		refresh();
	}
}
} // namespace le::input
