#include <engine/input/cursor_text.hpp>
#include <engine/input/state.hpp>
#include <engine/render/bitmap_font.hpp>
#include <engine/scene/prop.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::input {
struct CursorText::Pen : graphics::GlyphPen {
	using GlyphPen::GlyphPen;

	struct {
		std::string_view text{};
		std::size_t index{};
		f32 offsetX{};
		char cursor{};
	} data;

	void advancePedal(std::size_t idx, graphics::Geometry* out) override {
		if (idx == data.index || (data.index >= data.text.size() && idx >= data.text.size())) {
			f32 const offX = advanced() * data.offsetX;
			if (idx > 0 && offX != 0.0f) { position(head().x - offX, false); }
			write(static_cast<u8>(data.cursor), out);
			if (idx > 0 && offX != 0.0f) { position(head().x + offX, false); }
		}
	}
};

CursorText::CursorText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type) : BitmapText(font, vram, type), m_combined(vram, type) {}

bool CursorText::set(std::string_view text) {
	m_text = text;
	return refresh();
}

Span<Prop const> CursorText::props() const {
	m_prop = m_gen.prop(m_drawCursor ? m_combined : m_mesh, m_font->atlas());
	return m_prop;
}

bool CursorText::block(State const& state) {
	update(state);
	return !m_flags.test(Flag::eInactive);
}

void CursorText::pushSelf(Receivers& out) {
	Receiver::pushSelf(out);
	m_flags.reset(Flag::eInactive);
}

void CursorText::popSelf() noexcept {
	Receiver::popSelf();
	m_flags.set(Flag::eInactive);
}

void CursorText::backspace() {
	if (m_text.empty() || m_index == 0) { return; }
	if (m_index - 1 >= m_text.size()) {
		m_text.pop_back();
		if (m_index > 0) { --m_index; }
	} else {
		m_text.erase(m_index-- - 1, 1);
	}
	refresh();
}

void CursorText::deleteFront() {
	if (m_text.empty() || m_index >= m_text.size()) { return; }
	m_text.erase(m_index, 1);
	refresh();
}

void CursorText::insert(char ch) {
	if (m_index >= m_text.size()) {
		m_text += ch;
		if (m_index < npos) { ++m_index; }
	} else {
		m_text.insert(m_index++, 1, ch);
	}
	refresh();
}

void CursorText::update(State const& state) {
	if (state.pressed(Key::eEscape)) { deactivate(); }
	if (m_flags.test(Flag::eInactive)) {
		// hide cursor and ignore input
		m_drawCursor = false;
		return;
	}
	enum class post { none, cursor, refresh, regen }; // successive implies previous
	post post{};
	if (state.actions(Key::eBackspace).any(Actions(Action::ePressed, Action::eRepeated))) {
		backspace();
		post = post::cursor; // keep cursor visible
	}
	if (state.actions(Key::eDelete).any(Actions(Action::ePressed, Action::eRepeated))) {
		deleteFront();
		post = post::cursor; // keep cursor visible
	}
	if (!m_flags.test(Flag::eNoNewLine) && state.actions(Key::eEnter).any(Actions(Action::ePressed, Action::eRepeated))) {
		insert('\n');
		post = post::cursor; // keep cursor visible
	}
	if (state.actions(Key::eLeft).any(Actions(Action::ePressed, Action::eRepeated))) {
		if (m_index >= m_text.size()) {
			// reset to size - 1 if beyond it
			m_index = m_text.size() - 1;
		} else if (m_index > 0) {
			// decrement if > 0
			--m_index;
		}
		post = post::regen; // regenerate cursor text, keep cursor visible
	}
	if (state.actions(Key::eRight).any(Actions(Action::ePressed, Action::eRepeated))) {
		// increment if less than size
		if (m_index <= m_text.size()) { ++m_index; }
		post = post::regen; // regenerate cursor text, keep cursor visible
	}
	for (u32 const codepoint : state.codepoints) {
		// TODO: confirm codepoint range
		if (codepoint >= 32 && codepoint <= 255) {
			insert(static_cast<char>(codepoint));
			post = post::refresh; // refresh base and cursor text, keep cursor visible
		}
	}
	if (!m_flags.test(Flag::eNoAutoBlink)) {
		if (post >= post::cursor) {
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
	if (post == post::refresh) {
		refresh();
	} else if (post == post::regen) {
		regenerate();
	}
}

void CursorText::deactivate() noexcept { popSelf(); }

void CursorText::cursor(char ch) {
	if (ch != char{} && ch != m_cursor) {
		m_cursor = ch;
		regenerate();
	}
}

void CursorText::index(std::size_t index) {
	if (index != m_index) {
		m_index = index;
		regenerate();
	}
}

bool CursorText::refresh() {
	if (BitmapText::set(m_text)) {
		regenerate();
		return true;
	}
	return false;
}

void CursorText::regenerate() {
	if (m_text.empty()) { return; }
	Pen pen(&m_font->glyphs(), m_font->atlasSize(), m_gen.size, m_gen.position, m_gen.colour);
	pen.data = {m_text, m_index, m_offsetX, m_cursor};
	auto const lines = pen.splitLines(m_text);
	auto const align = m_gen.align - 0.5f;
	graphics::Geometry combined;
	auto para = pen.writeLines(lines, 5.0f);
	// apply offsets per line: line width, total height
	for (auto& [line, extent] : para.lines) {
		line.offset({align.x * extent.x, align.y * para.height, 0.0f});
		combined.append(line);
	}
	m_combined.construct(std::move(combined));
}
} // namespace le::input
