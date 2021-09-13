#include <engine/gui/widget.hpp>

namespace le::gui {
Widget::Widget(not_null<TreeRoot*> root, Hash style) : Quad(root, true) {
	m_style = Styles::get(style);
	m_rect.size = {50.0f, 50.0f};
}

Widget::Status Widget::status(input::State const& state) const noexcept {
	if (m_interact && hit(state.cursor.position)) {
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

Widget::Status Widget::onInput(input::State const& state) {
	if (!m_active) { return Status::eInactive; }
	Status ret;
	if (clicked(state, true, &ret)) { m_onClick(); }
	forEachNode<Widget>(&Widget::onInput, state);
	return ret;
}

bool Widget::clickedImpl(bool style, Status st) noexcept {
	bool const cooldown = m_previous.point != time::Point() && time::diff(m_previous.point) < m_debounce;
	if (style && (!cooldown || st <= Status::eHover)) { m_material = m_style.widget.quad[st]; }
	auto const prev = std::exchange(m_previous.status, st);
	bool const ret = !cooldown && st == Status::eRelease && (prev >= Status::ePress && prev <= Status::eHold);
	if (ret) { m_previous.point = time::now(); }
	return ret;
}
} // namespace le::gui
