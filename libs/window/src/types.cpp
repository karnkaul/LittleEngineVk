#include <window/types.hpp>

#if defined(LEVK_USE_GLFW)
#include <GLFW/glfw3.h>
#endif

namespace le {
std::string_view window::Joystick::name() const { return joystickName(id); }

f32 window::Gamepad::axis(Axis axis) const {
	[[maybe_unused]] std::size_t idx = std::size_t(axis);
#if defined(LEVK_USE_GLFW)
	s32 max = 0;
	glfwGetJoystickAxes(id, &max);
	if (idx < (std::size_t)max && idx < axes.size()) { return axes[idx]; }
#endif
	return 0.0f;
}

bool window::Gamepad::pressed(Key button) const {
	[[maybe_unused]] std::size_t idx = (std::size_t)button - (std::size_t)Key::eGamepadButtonA;
#if defined(LEVK_USE_GLFW)
	s32 max = 0;
	glfwGetJoystickButtons(id, &max);
	if (idx < (std::size_t)max && idx < buttons.size()) { return buttons[idx]; }
#endif
	return false;
}

std::string_view window::toString([[maybe_unused]] s32 key) {
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
