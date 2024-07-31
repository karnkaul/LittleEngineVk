#include <levk/components/free_camera.hpp>
#include <levk/imcpp/im_controls.hpp>
#include <levk/imcpp/im_text.hpp>
#include <levk/util.hpp>

namespace levk {
void FreeCamera::tick(Entity& entity, Seconds const dt) {
	auto const& input_state = entity.get_input_state();

	m_control = input_state.mouse_buttons.is_held(control_mouse_button);
	auto d_move = glm::vec3{};
	if (m_control) {
		d_move = control(input_state, dt);
	} else {
		m_prev_cursor.reset();
	}

	entity.transform.set_position(entity.transform.get_position() + move_speed * d_move * dt.count());
	entity.transform.set_orientation(glm::vec3{glm::radians(pitch.value), glm::radians(yaw.value), 0.0f});
}

auto FreeCamera::control(input::State const& input_state, Seconds const dt) -> glm::vec3 {
	auto const dspeed = [dy = input_state.mouse_scroll.y] {
		if (dy < 0.0f) { return 0.9f; }
		if (dy > 0.0f) { return 1.1f; }
		return 0.0f;
	}();
	if (std::abs(dspeed) > 0.0f) { move_speed = std::clamp(move_speed * dspeed, 0.01f, 10000.0f); }

	if (!m_prev_cursor) { m_prev_cursor = input_state.cursor_position; }
	auto const d_cursor = input_state.cursor_position - *m_prev_cursor;
	auto const d_look = look_speed * d_cursor * dt.count();
	yaw = std::lerp(yaw.value, yaw.value - d_look.x, look_lerp);
	pitch = std::lerp(pitch.value, pitch.value + d_look.y, look_lerp);

	auto const d_xyz = [&input_state] {
		auto ret = glm::vec3{};
		if (input_state.keys.is_held(GLFW_KEY_W)) { ret.z -= 1.0f; }
		if (input_state.keys.is_held(GLFW_KEY_A)) { ret.x -= 1.0f; }
		if (input_state.keys.is_held(GLFW_KEY_S)) { ret.z += 1.0f; }
		if (input_state.keys.is_held(GLFW_KEY_D)) { ret.x += 1.0f; }
		if (input_state.keys.is_held(GLFW_KEY_Q)) { ret.y -= 1.0f; }
		if (input_state.keys.is_held(GLFW_KEY_E)) { ret.y += 1.0f; }
		if (std::abs(ret.x) > 0.0f || std::abs(ret.y) > 0.0f || std::abs(ret.z) > 0.0f) { ret = glm::normalize(ret); }
		return ret;
	}();
	auto const camera_orn = levk::Transform::to_orientation(yaw, pitch);
	auto const camera_fwd = camera_orn * levk::front_v;
	auto const camera_right = camera_orn * levk::right_v;

	m_prev_cursor = input_state.cursor_position;

	return d_xyz.z * camera_fwd + d_xyz.x * camera_right + d_xyz.y * levk::up_v;
}

void FreeCamera::im_inspect() {
	ISceneCamera::im_inspect();

	auto rotation = glm::vec2{pitch.value, yaw.value};
	if (ImDragFloat{}.fvec("rotation", rotation)) {
		pitch.value = rotation.x;
		yaw.value = rotation.y;
	}

	auto drag_float = ImDragFloat{.range = {1.0f, 5000.0f}, .reset_button = false};
	drag_float.f32("move speed", move_speed, 1.0f);
	drag_float.f32("look speed", look_speed, 1.0f);
	drag_float.range = {0.1f, 1.0f};
	drag_float.f32("look lerp", look_lerp, 0.5f);
}
} // namespace levk
