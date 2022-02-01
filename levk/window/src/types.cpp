#include <levk/window/types.hpp>

#if defined(LEVK_USE_GLFW)
#include <GLFW/glfw3.h>
#endif

namespace le {
std::string_view window::Joystick::name() const { return joystickName(id); }

f32 window::Gamepad::axis(int axis) const noexcept {
#if defined(LEVK_USE_GLFW)
	int max{};
	glfwGetJoystickAxes(id, &max);
	if (axis < max && axis < (int)axes.size()) { return axes[axis]; }
#endif
	return 0.0f;
}

bool window::Gamepad::pressed(u8 button) const noexcept {
#if defined(LEVK_USE_GLFW)
	int max{};
	glfwGetJoystickButtons(id, &max);
	if ((int)button < max && button < buttons.size()) { return buttons[button]; }
#endif
	return false;
}

std::string_view window::toString([[maybe_unused]] int key) {
	static constexpr std::string_view unknown = "(Unknown)";
#if defined(LEVK_USE_GLFW)
	char const* szName = glfwGetKeyName((int)key, 0);
	return szName ? std::string_view(szName) : unknown;
#else
	return unknown;
#endif
}

std::string_view window::joystickName(int id) {
#if defined(LEVK_USE_GLFW)
	if (glfwJoystickIsGamepad(id)) { return glfwGetGamepadName(id); }
	if (auto const name = glfwGetJoystickName(id)) { return name; }
#endif
	return {};
}
} // namespace le
