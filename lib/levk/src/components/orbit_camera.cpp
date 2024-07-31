#include <levk/components/orbit_camera.hpp>
#include <levk/imcpp/im_controls.hpp>
#include <levk/imcpp/im_text.hpp>
#include <levk/util.hpp>

namespace levk {
void OrbitCamera::tick(Entity& entity, levk::Seconds dt) {
	auto const& input_state = entity.get_input_state();
	m_control = input_state.mouse_buttons.is_held(control_mouse_button) && !ImGui::GetIO().WantCaptureMouse;

	control(input_state, dt);

	auto direction = front_v;
	auto const q_pitch = glm::angleAxis(glm::radians(pitch.value), right_v);
	auto const q_yaw = glm::angleAxis(glm::radians(yaw.value), up_v);
	direction = q_pitch * direction;
	direction = glm::normalize(q_yaw * direction);

	entity.transform.set_position(radius * direction);
	entity.transform.set_orientation(q_yaw * q_pitch);
}

void OrbitCamera::control(input::State const& input_state, Seconds const dt) {
	auto const dradius = zoom_speed * -input_state.mouse_scroll.y;
	radius = std::clamp(radius + dradius, 0.01f, 10000.0f);

	if (!m_control) {
		m_prev_cursor.reset();
		return;
	}

	if (!m_prev_cursor) { m_prev_cursor = input_state.cursor_position; }
	auto const d_cursor = input_state.cursor_position - *m_prev_cursor;
	auto const d_look = orbit_rate.value * d_cursor * dt.count();
	yaw.value -= d_look.x;
	pitch.value += d_look.y;

	auto direction = front_v;
	auto const q_pitch = glm::angleAxis(glm::radians(pitch.value), right_v);
	auto const q_yaw = glm::angleAxis(glm::radians(yaw.value), up_v);
	direction = q_pitch * direction;
	direction = glm::normalize(q_yaw * direction);

	m_prev_cursor = input_state.cursor_position;
}

void OrbitCamera::im_inspect() {
	ISceneCamera::im_inspect();

	auto rotation = glm::vec2{pitch.value, yaw.value};
	if (ImDragFloat{}.fvec("rotation", rotation)) {
		pitch.value = rotation.x;
		yaw.value = rotation.y;
	}

	auto drag_float = ImDragFloat{};
	drag_float.fvec("origin", origin);
	drag_float.range = {0.5f, 1000.0f};
	drag_float.reset_button = false;
	drag_float.f32("radius", radius);
	drag_float.f32("orbit rate", orbit_rate.value);
	drag_float.range.hi = 100.0f;
	drag_float.f32("zoom speed", zoom_speed);
}
} // namespace levk
