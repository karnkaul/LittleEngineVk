#include <engine/gui/widget.hpp>

namespace le::gui {
Widget::Widget(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) : Quad(root, true) {
	m_rect.size = {50.0f, 50.0f};
	m_text = &push<Text>(font);
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

bool Widget::clicked(input::State const& state, bool style, Status* out) noexcept {
	auto const st = status(state);
	if (out) { *out = st; }
	return clickedImpl(style, st);
}

void Widget::refresh(input::State const* state) {
	m_previous.set = false;
	if (state) {
		clicked(*state);
	} else {
		clickedImpl(true, Status::eIdle);
	}
}

Status Widget::onInput(input::State const& state) {
	Status ret;
	if (clicked(state, true, &ret)) { m_onClick(); }
	return ret;
}

bool Widget::clickedImpl(bool style, Status st) noexcept {
	bool const cooldown = m_previous.point != time::Point() && time::diff(m_previous.point) < m_debounce;
	if (style && (!cooldown || st <= Status::eHover)) {
		m_material = m_styles.quad[st];
		if (m_text && (!m_previous.set || st != m_previous.status)) {
			Text::Factory factory = m_text->factory();
			factory.colour = m_styles.text[st];
			m_text->set(std::move(factory));
			m_previous.set = true;
		}
	}
	auto const prev = std::exchange(m_previous.status, st);
	bool const ret = !cooldown && st == Status::eRelease && (prev >= Status::ePress && prev <= Status::eHold);
	if (ret) { m_previous.point = time::now(); }
	return ret;
}
} // namespace le::gui
