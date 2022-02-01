#pragma once
#include <levk/core/span.hpp>
#include <levk/core/std_types.hpp>
#include <string_view>

namespace le::window {
enum class CursorType : s8 { eDefault = 0, eResizeEW, eResizeNS, eResizeNWSE, eResizeNESW, eCOUNT_ };
enum class CursorMode : s8 { eDefault = 0, eHidden, eDisabled };

struct Joystick {
	std::array<f32, 6> axes{};
	std::array<u8, 15> buttons{};
	int id = 0;

	std::string_view name() const;
};

struct Gamepad : Joystick {
	f32 axis(int axis) const noexcept;
	bool pressed(u8 button) const noexcept;
};

constexpr f32 triggerToAxis(f32 triggerValue) noexcept;
std::string_view toString(int key);
std::string_view joystickName(int id);
} // namespace le::window

// impl

constexpr le::f32 le::window::triggerToAxis(f32 triggerValue) noexcept { return (triggerValue + 1.0f) * 0.5f; }
