#pragma once
#include <le/input/action.hpp>
#include <array>

namespace le::input {
struct Gamepad {
	static constexpr std::size_t max_buttons_v{15};
	static constexpr std::size_t max_axes_v{6};

	std::array<Action, max_buttons_v> buttons{}; // key: GLFW_GAMEPAD_BUTTON_*
	std::array<float, max_axes_v> axes{};		 // key: GLFW_GAMEPAD_AXIS_*

	bool is_connected{};

	explicit operator bool() const { return is_connected; }
};
} // namespace le::input
