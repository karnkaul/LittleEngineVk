#include <unordered_map>
#if defined(LEVK_USE_GLFW)
#include <GLFW/glfw3.h>
#endif
#include <engine/window/input_types.hpp>

namespace le
{
namespace
{
using namespace input;

std::unordered_map<std::string_view, Key> const g_keyMap = {
	{"space", Key::eSpace},
	{"'", Key::eApostrophe},
	{",", Key::eComma},
	{"-", Key::eMinus},
	{".", Key::eFullstop},
	{"/", Key::eSlash},
	{"1", Key::e1},
	{"2", Key::e2},
	{"3", Key::e3},
	{"4", Key::e4},
	{"5", Key::e5},
	{"6", Key::e6},
	{"7", Key::e7},
	{"8", Key::e8},
	{"9", Key::e9},
	{";", Key::eSemicolon},
	{"=", Key::eEqual},
	{"a", Key::eA},
	{"b", Key::eB},
	{"c", Key::eC},
	{"d", Key::eD},
	{"e", Key::eE},
	{"f", Key::eF},
	{"g", Key::eG},
	{"h", Key::eH},
	{"i", Key::eI},
	{"j", Key::eJ},
	{"k", Key::eK},
	{"l", Key::eL},
	{"m", Key::eM},
	{"n", Key::eN},
	{"o", Key::eO},
	{"p", Key::eP},
	{"q", Key::eQ},
	{"r", Key::eR},
	{"s", Key::eS},
	{"t", Key::eT},
	{"u", Key::eU},
	{"v", Key::eV},
	{"w", Key::eW},
	{"x", Key::eX},
	{"y", Key::eY},
	{"z", Key::eZ},
	{"[", Key::eLeftBracket},
	{"\\", Key::eBackslash},
	{"]", Key::eRightBracket},
	{"`", Key::eGrave},
	{"esc", Key::eEscape},
	{"enter", Key::eEnter},
	{"tab", Key::eTab},
	{"backspace", Key::eBackspace},
	{"insert", Key::eInsert},
	{"delete", Key::eDelete},
	{"right", Key::eRight},
	{"left", Key::eLeft},
	{"down", Key::eDown},
	{"up", Key::eUp},
	{"page_up", Key::ePageUp},
	{"page_down", Key::ePageDown},
	{"home", Key::eHome},
	{"end", Key::eEnd},
	{"caps_lock", Key::eCapsLock},
	{"scroll_lock", Key::eScrollLock},
	{"num_lock", Key::eNumLock},
	{"print_screen", Key::ePrintScreen},
	{"pause", Key::ePause},
	{"f1", Key::eF1},
	{"f2", Key::eF2},
	{"f3", Key::eF3},
	{"f4", Key::eF4},
	{"f5", Key::eF5},
	{"f6", Key::eF6},
	{"f7", Key::eF7},
	{"f8", Key::eF8},
	{"f9", Key::eF9},
	{"f10", Key::eF10},
	{"f11", Key::eF11},
	{"f12", Key::eF12},
	{"f13", Key::eF13},
	{"f14", Key::eF14},
	{"f15", Key::eF15},
	{"f16", Key::eF16},
	{"f17", Key::eF17},
	{"f18", Key::eF18},
	{"f19", Key::eF19},
	{"f20", Key::eF20},
	{"f21", Key::eF21},
	{"f22", Key::eF22},
	{"f23", Key::eF23},
	{"f24", Key::eF24},
	{"f25", Key::eF25},
	{"keypad_0", Key::eKp0},
	{"keypad_1", Key::eKp1},
	{"keypad_2", Key::eKp2},
	{"keypad_3", Key::eKp3},
	{"keypad_4", Key::eKp4},
	{"keypad_5", Key::eKp5},
	{"keypad_6", Key::eKp6},
	{"keypad_7", Key::eKp7},
	{"keypad_8", Key::eKp8},
	{"keypad_9", Key::eKp9},
	{"keypad_enter", Key::eKpEnter},
	{"left_shift", Key::eLeftShift},
	{"left_ctrl", Key::eLeftControl},
	{"left_alt", Key::eLeftAlt},
	{"left_super", Key::eLeftSuper},
	{"right_shift", Key::eRightShift},
	{"right_ctrl", Key::eRightControl},
	{"right_alt", Key::eRightAlt},
	{"right_super", Key::eRightSuper},
	{"menu", Key::eMenu},
	{"mouse_1", Key::eMouseButton1},
	{"mouse_2", Key::eMouseButton2},
	{"mouse_middle", Key::eMouseButtonMiddle},
	{"gamepad_a", Key::eGamepadButtonA},
	{"gamepad_b", Key::eGamepadButtonB},
	{"gamepad_x", Key::eGamepadButtonX},
	{"gamepad_y", Key::eGamepadButtonY},
	{"gamepad_lb", Key::eGamepadButtonLeftBumper},
	{"gamepad_rb", Key::eGamepadButtonRightBumper},
	{"gamepad_back", Key::eGamepadButtonBack},
	{"gamepad_start", Key::eGamepadButtonStart},
	{"gamepad_guide", Key::eGamepadButtonGuide},
	{"gamepad_right_thumb", Key::eGamepadButtonLeftThumb},
	{"gamepad_left_thumb", Key::eGamepadButtonRightThumb},
	{"gamepad_dpad_up", Key::eGamepadButtonDpadUp},
	{"gamepad_dpad_right", Key::eGamepadButtonDpadRight},
	{"gamepad_dpad_down", Key::eGamepadButtonDpadDown},
	{"gamepad_dpad_left", Key::eGamepadButtonDpadLeft},
};

std::unordered_map<std::string_view, Mods::VALUE> const g_modsMap = {{"ctrl", Mods::eCONTROL}, {"alt", Mods::eALT}, {"shift", Mods::eSHIFT}};

std::unordered_map<std::string_view, Axis> const g_axisMap = {
	{"gamepad_left_x", Axis::eLeftX},	{"gamepad_left_y", Axis::eLeftY},	{"gamepad_right_x", Axis::eRightX},
	{"gamepad_right_y", Axis::eRightY}, {"gamepad_lt", Axis::eLeftTrigger}, {"gamepad_rt", Axis::eRightTrigger},
};

template <typename T>
T parse(std::unordered_map<std::string_view, T> const& map, std::string_view str, T fail)
{
	if (auto search = map.find(str); search != map.end() && !str.empty())
	{
		return search->second;
	}
	return fail;
}
} // namespace

bool input::isMouseButton(Key key)
{
	return (s32)key >= (s32)Key::eMouseButtonBegin && (s32)key < (s32)Key::eMouseButtonEnd;
}

bool input::isGamepadButton(Key key)
{
	auto ret = (s32)key >= (s32)Key::eGamepadButtonBegin && (s32)key < (s32)Key::eGamepadButtonEnd;
	return ret;
}

bool input::isMouseAxis(Axis axis)
{
	return (s32)axis >= (s32)Axis::eMouseBegin && (s32)axis < (s32)Axis::eMouseEnd;
}

Key input::parseKey(std::string_view str)
{
	return parse(g_keyMap, str, Key::eUnknown);
}

Action input::parseAction(std::string_view str)
{
	if (str == "press")
	{
		return Action::ePress;
	}
	return Action::eRelease;
}

Mods::VALUE input::parseMods(Span<std::string> vec)
{
	s32 ret = 0;
	for (auto const& str : vec)
	{
		ret |= (s32)parse(g_modsMap, str, Mods::eNONE);
	}
	return static_cast<Mods::VALUE>(ret);
}

Axis input::parseAxis(std::string_view str)
{
	return parse(g_axisMap, str, Axis::eUnknown);
}

std::vector<Gamepad> input::activeGamepads()
{
	std::vector<Gamepad> ret;
#if defined(LEVK_USE_GLFW)
	for (s32 id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id)
	{
		GLFWgamepadstate state;
		if (glfwJoystickPresent(id) && glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &state))
		{
			Gamepad padi;
			padi.name = glfwGetGamepadName(id);
			padi.id = id;
			padi.joyState.buttons = std::vector<u8>(15, 0);
			padi.joyState.axes = std::vector<f32>(6, 0);
			std::memcpy(padi.joyState.buttons.data(), state.buttons, padi.joyState.buttons.size());
			std::memcpy(padi.joyState.axes.data(), state.axes, padi.joyState.axes.size() * sizeof(f32));
			ret.push_back(std::move(padi));
		}
	}
#endif
	return ret;
}

Joystick input::joyState([[maybe_unused]] s32 id)
{
	Joystick ret;
#if defined(LEVK_USE_GLFW)
	if (glfwJoystickPresent(id))
	{
		ret.id = id;
		s32 count;
		auto const axes = glfwGetJoystickAxes(id, &count);
		ret.axes.reserve((std::size_t)count);
		for (s32 idx = 0; idx < count; ++idx)
		{
			ret.axes.push_back(axes[idx]);
		}
		auto const buttons = glfwGetJoystickButtons(id, &count);
		ret.buttons.reserve((std::size_t)count);
		for (s32 idx = 0; idx < count; ++idx)
		{
			ret.buttons.push_back(buttons[idx]);
		}
		auto const szName = glfwGetJoystickName(id);
		if (szName)
		{
			ret.name = szName;
		}
	}
#endif
	return ret;
}

Gamepad input::gamepadState([[maybe_unused]] s32 id)
{
	Gamepad ret;
#if defined(LEVK_USE_GLFW)
	GLFWgamepadstate state;
	if (glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &state))
	{
		ret.name = glfwGetGamepadName(id);
		ret.id = id;
		ret.joyState.buttons = std::vector<u8>(15, 0);
		ret.joyState.axes = std::vector<f32>(6, 0);
		std::memcpy(ret.joyState.buttons.data(), state.buttons, ret.joyState.buttons.size());
		std::memcpy(ret.joyState.axes.data(), state.axes, ret.joyState.axes.size() * sizeof(f32));
	}
#endif
	return ret;
}

f32 input::triggerToAxis([[maybe_unused]] f32 triggerValue)
{
	f32 ret = triggerValue;
#if defined(LEVK_USE_GLFW)
	ret = (triggerValue + 1.0f) * 0.5f;
#endif
	return ret;
}

std::size_t input::joystickAxesCount([[maybe_unused]] s32 id)
{
	std::size_t ret = 0;
#if defined(LEVK_USE_GLFW)
	s32 max;
	glfwGetJoystickAxes(id, &max);
	ret = std::size_t(max);
#endif
	return ret;
}

std::size_t input::joysticKButtonsCount([[maybe_unused]] s32 id)
{
	std::size_t ret = 0;
#if defined(LEVK_USE_GLFW)
	s32 max;
	glfwGetJoystickButtons(id, &max);
	ret = std::size_t(max);
#endif
	return ret;
}

std::string_view input::toString([[maybe_unused]] s32 key)
{
	static const std::string_view blank = "";
	std::string_view ret = blank;
#if defined(LEVK_USE_GLFW)
	ret = glfwGetKeyName(key, 0);
#endif
	return ret;
}
} // namespace le
