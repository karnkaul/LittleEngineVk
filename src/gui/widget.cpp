#include <engine/gui/widget.hpp>

namespace le::gui {
Widget::Widget(not_null<TreeRoot*> root, not_null<graphics::VRAM*> vram, not_null<BitmapFont const*> font) : Quad(root, vram) {
	m_rect.size = {50.0f, 50.0f};
	m_text = &push<Text>(vram, font);
	m_styles.text.base = colours::black;
}

Status Widget::status(input::State const& state) const noexcept {
	if (hit(state.cursor.position)) {
		auto const actions = state.actions(input::Key::eMouseButton1);
		if (actions.all(input::Action::eReleased)) {
			return Status::eRelease;
		} else if (actions.all(input::Action::ePressed)) {
			return Status::ePress;
		} else if (actions.all(input::Action::eHeld)) {
			return Status::eHold;
		}
		return Status::eHover;
	}
	return Status::eIdle;
}

bool Widget::clicked(input::State const& state, bool style) noexcept {
	bool const cooldown = m_previous.point != time::Point() && time::diff(m_previous.point) < m_debounce;
	auto const st = status(state);
	if (style && (!cooldown || st <= Status::eHover)) {
		m_material = m_styles.quad[st];
		if (m_text) { m_text->m_text.colour = m_styles.text[st]; }
	}
	auto const prev = std::exchange(m_previous.status, st);
	bool const ret = !cooldown && st == Status::eRelease && (prev >= Status::ePress && prev <= Status::eHold);
	if (ret) { m_previous.point = time::now(); }
	return ret;
}
} // namespace le::gui
