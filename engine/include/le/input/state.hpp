#pragma once
#include <glm/vec2.hpp>
#include <le/input/gamepad.hpp>
#include <array>
#include <string>
#include <vector>

namespace le::input {
struct State {
	enum Property : std::uint32_t {
		eWindowSize,
		eFramebufferSize,
		eCursorPosition,
		eFocus,
	};

	static constexpr std::size_t max_keys_v{512};
	static constexpr std::size_t max_mouse_buttons_v{8};
	static constexpr std::size_t max_gamepads_v{16};

	std::array<Action, max_keys_v> keyboard{};				 // key: GLFW_KEY_*
	std::array<Action, max_mouse_buttons_v> mouse_buttons{}; // key: GLFW_MOUSE_BUTTON_*
	std::array<Gamepad, max_gamepads_v> gamepads{};			 // key: GLFW_JOYSTICK_*
	std::vector<std::string> drops{};

	glm::uvec2 window_extent{};
	glm::uvec2 framebuffer_extent{};
	glm::vec2 cursor_position{};
	glm::vec2 raw_cursor_position{};

	bool in_focus{};
	bool shutting_down{};

	std::uint32_t changed{};
	std::size_t last_engaged_gamepad_index{};

	[[nodiscard]] constexpr auto display_ratio() const -> glm::vec2 {
		if (window_extent.x == 0 || window_extent.y == 0 || framebuffer_extent.x == 0 || framebuffer_extent.y == 0) { return {}; }
		return glm::vec2{framebuffer_extent} / glm::vec2{window_extent};
	}
};
} // namespace le::input
