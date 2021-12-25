#include <engine/input/state.hpp>
#include <engine/input/text_cursor.hpp>
#include <graphics/font/font.hpp>

namespace le::input {
TextCursor2::TextCursor2(not_null<Font*> font, Flags flags) : m_flags(flags), m_primitive(font->m_vram), m_font(font) { refresh(); }

MeshView TextCursor2::mesh() const noexcept {
	if (m_drawCursor) {
		m_material.Tf = m_colour;
		m_material.d = m_alpha;
		return MeshObj{&m_primitive, &m_material};
	}
	return {};
}

void TextCursor2::backspace(graphics::Geometry* out) {
	if (m_line.empty() || m_index == 0) {
		refresh(out);
		return;
	}
	if (m_index - 1 >= m_line.size()) {
		m_line.pop_back();
		if (m_index > 0) { --m_index; }
	} else {
		m_line.erase(m_index-- - 1, 1);
	}
	refresh();
}

void TextCursor2::deleteFront(graphics::Geometry* out) {
	if (!m_line.empty() && m_index < m_line.size()) { m_line.erase(m_index, 1); }
	refresh(out);
}

void TextCursor2::insert(char ch, graphics::Geometry* out) {
	if (m_index >= m_line.size()) {
		m_line += ch;
		if (m_index < npos) { ++m_index; }
	} else {
		m_line.insert(m_index++, 1, ch);
	}
	refresh(out);
}

bool TextCursor2::update(State const& state, graphics::Geometry* out, bool clearGeom) {
	if (state.pressed(Key::eEscape)) { setActive(false); }
	if (!m_flags.test(Flag::eActive)) {
		// hide cursor and ignore input
		m_drawCursor = false;
		return false;
	}
	bool regen = {};
	if (state.pressOrRepeat(Key::eBackspace)) {
		backspace(nullptr);
		regen = true;
	}
	if (state.pressOrRepeat(Key::eDelete)) {
		deleteFront(nullptr);
		regen = true;
	}
	// if (!m_flags.test(Flag::eNoNewLine) && state.pressOrRepeat(Key::eEnter)) {
	// 	insert('\n', nullptr);
	// 	refr = true;
	// }
	if (state.pressOrRepeat(Key::eLeft)) {
		if (m_index >= m_line.size()) {
			// reset to size - 1 if beyond it
			m_index = m_line.size() - 1;
		} else if (m_index > 0) {
			// decrement if > 0
			--m_index;
		}
		regen = true;
	}
	if (state.pressOrRepeat(Key::eRight)) {
		// increment if less than size
		if (m_index <= m_line.size()) { ++m_index; }
		regen = true;
	}
	if (state.pressed(Key::eHome)) {
		m_index = 0U;
		regen = true;
	}
	if (state.pressed(Key::eEnd)) {
		m_index = npos;
		regen = true;
	}
	for (u32 const codepoint : state.codepoints) {
		if (Codepoint::Validate{}(codepoint)) {
			insert(static_cast<char>(codepoint), nullptr);
			regen = true;
		}
	}
	if (!m_flags.test(Flag::eNoAutoBlink)) {
		if (regen) {
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
	refresh(out, clearGeom, regen);
	return regen;
}

void TextCursor2::refresh(graphics::Geometry* out, bool clearGeom) { refresh(out, clearGeom, true); }

graphics::Geometry TextCursor2::generateText() {
	graphics::Geometry ret;
	refresh(&ret);
	return ret;
}

void TextCursor2::setActive(bool active) noexcept {
	m_drawCursor = active;
	m_flags.assign(Flag::eActive, active);
}

void TextCursor2::index(std::size_t index) {
	if (index != m_index) {
		m_index = index;
		refresh();
	}
}

void TextCursor2::refresh(graphics::Geometry* out, bool clearGeom, bool regen) {
	if (out) {
		if (clearGeom) { *out = {}; }
		out->reserve(u32(m_line.size() * 4U), u32(m_line.size() * 6U));
	}
	if (out || regen) {
		m_layout.pivot.y = -0.5f;
		Font::PenInfo const info{m_layout.origin, m_layout.scale, m_layout.lineSpacing, out};
		Font::Pen pen(m_font, info);
		auto const size = m_layout.scale * m_size * glm::vec2(f32(m_font->face().height()));
		auto const head = pen.writeLine(m_line, m_layout.pivot, &m_index);
		if (regen) {
			m_position = head;
			m_position.y += 0.3f * size.y;
			graphics::GeomInfo const gi{.origin = m_position};
			m_primitive.construct(graphics::makeQuad(size, gi));
		}
	}
}
} // namespace le::input
