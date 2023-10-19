#include <le/engine.hpp>
#include <le/scene/ui/interactable.hpp>

namespace le::ui {
void Interactable::update(graphics::Rect2D<> const& hitbox) {
	auto const extent = hitbox.extent();
	auto const hit_test = [hitbox, extent](glm::vec2 const point) {
		if (extent.x <= 0.0f || extent.y <= 0.0f) { return false; };
		return hitbox.contains(point);
	};

	auto const& input_state = Engine::self().input_state();
	auto const in_focus = hit_test(input_state.cursor_position);

	if (!m_in_focus && in_focus) {
		on_focus_gained();
	} else if (!in_focus && m_in_focus) {
		on_focus_lost();
	}
	m_in_focus = in_focus;

	if (!m_in_focus) { return; }

	auto const mb1_state = input_state.mouse_buttons.at(GLFW_MOUSE_BUTTON_1);
	using enum input::Action;
	switch (mb1_state) {
	case ePress: on_press(); break;
	case eRelease: on_release(); break;
	default: break;
	}
}
} // namespace le::ui
