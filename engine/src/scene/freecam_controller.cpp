#include <imgui.h>
#include <spaced/engine.hpp>
#include <spaced/scene/entity.hpp>
#include <spaced/scene/freecam_controller.hpp>
#include <spaced/scene/scene.hpp>

#include <spaced/core/logger.hpp>

namespace spaced {
namespace {
auto const g_log{logger::Logger{"Freecam"}};
}

void FreecamController::tick(Duration dt) {
	auto data = get_scene().camera.transform.data();
	auto* window = Engine::self().window();
	auto const& input = Engine::self().input_state();

	if (input.mouse_buttons[GLFW_MOUSE_BUTTON_RIGHT] == input::Action::eHold) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	auto dxy = glm::vec2{};
	auto const input_mode = glfwGetInputMode(window, GLFW_CURSOR);
	g_log.debug("input mode: {}", input_mode);

	if (input_mode != GLFW_CURSOR_DISABLED) {
		m_prev_cursor = input.cursor_position;
	} else {
		dxy = input.cursor_position - m_prev_cursor;
		dxy.y = -dxy.y;
		auto const d_pitch_yaw = look_speed * glm::vec2{glm::radians(dxy.y), glm::radians(dxy.x)};
		pitch.value -= d_pitch_yaw.x;
		yaw.value -= d_pitch_yaw.y;

		auto dxyz = glm::vec3{};
		auto front = data.orientation * front_v;
		auto right = data.orientation * right_v;
		auto up = data.orientation * up_v;

		if (input.keyboard[GLFW_KEY_W] == input::Action::eHold) { dxyz.z -= 1.0f; }
		if (input.keyboard[GLFW_KEY_S] == input::Action::eHold) { dxyz.z += 1.0f; }
		if (input.keyboard[GLFW_KEY_A] == input::Action::eHold) { dxyz.x -= 1.0f; }
		if (input.keyboard[GLFW_KEY_D] == input::Action::eHold) { dxyz.x += 1.0f; }
		if (input.keyboard[GLFW_KEY_Q] == input::Action::eHold) { dxyz.y -= 1.0f; }
		if (input.keyboard[GLFW_KEY_E] == input::Action::eHold) { dxyz.y += 1.0f; }
		if (std::abs(dxyz.x) > 0.0f || std::abs(dxyz.y) > 0.0f || std::abs(dxyz.z) > 0.0f) {
			dxyz = glm::normalize(dxyz);
			auto const factor = dt.count() * move_speed;
			data.position += factor * front * dxyz.z;
			data.position += factor * right * dxyz.x;
			data.position += factor * up * dxyz.y;
		}
	}

	data.orientation = glm::vec3{pitch, yaw, 0.0f};
	m_prev_cursor = input.cursor_position;

	get_scene().camera.transform.set_data(data);
}
} // namespace spaced
