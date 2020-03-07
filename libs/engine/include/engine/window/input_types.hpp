#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "core/std_types.hpp"
#include "core/delegate.hpp"

namespace le
{
enum class Action
{
	RELEASE = 0,
	PRESS = 1,
	REPEAT = 2,
};

enum class Key
{
	UNKNOWN = -1,
	SPACE = 32,
	APOSTROPHE = 39,
	COMMA = 44,
	MINUS = 45,
	FULLSTOP = 46,
	SLASH = 47,
	N0 = 48,
	N1 = 49,
	N2 = 50,
	N3 = 51,
	N4 = 52,
	N5 = 53,
	N6 = 54,
	N7 = 55,
	N8 = 56,
	N9 = 57,
	SEMICOLON = 59,
	EQUAL = 61,
	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90,
	LEFT_BRACKET = 91,
	BACKSLASH = 92,
	RIGHT_BRACKET = 93,
	GRAVE = 94,
	BACKTICK = GRAVE,

	ESCAPE = 256,
	ENTER = 257,
	TAB = 258,
	BACKSPACE = 259,
	INSERT = 260,
	DELETE = 261,
	RIGHT = 262,
	LEFT = 263,
	DOWN = 264,
	UP = 265,
	PAGE_UP = 266,
	PAGE_DOWN = 267,
	HOME = 268,
	END = 269,
	CAPS_LOCK = 280,
	SCROLL_LOCK = 281,
	NUM_LOCK = 282,
	PRINT_SCREEN = 283,
	PAUSE = 284,
	F1 = 290,
	F2 = 291,
	F3 = 292,
	F4 = 293,
	F5 = 294,
	F6 = 295,
	F7 = 296,
	F8 = 297,
	F9 = 298,
	F10 = 299,
	F11 = 300,
	F12 = 301,
	F13 = 302,
	F14 = 303,
	F15 = 304,
	F16 = 305,
	F17 = 306,
	F18 = 307,
	F19 = 308,
	F20 = 309,
	F21 = 310,
	F22 = 311,
	F23 = 312,
	F24 = 313,
	F25 = 314,
	KP_0 = 320,
	KP_1 = 321,
	KP_2 = 322,
	KP_3 = 323,
	KP_4 = 324,
	KP_5 = 325,
	KP_6 = 326,
	KP_7 = 327,
	KP_8 = 328,
	KP_9 = 329,
	KP_DECIMAL = 330,
	KP_DIVIDE = 331,
	KP_MULTIPLY = 332,
	KP_SUBTRACT = 333,
	KP_ADD = 334,
	KP_ENTER = 335,
	KP_EQUAL = 336,
	LEFT_SHIFT = 340,
	LEFT_CONTROL = 341,
	LEFT_ALT = 342,
	LEFT_SUPER = 343,
	RIGHT_SHIFT = 344,
	RIGHT_CONTROL = 345,
	RIGHT_ALT = 346,
	RIGHT_SUPER = 347,
	MENU = 348,

	MOUSE_BUTTON_1 = 500,
	MOUSE_BUTTON_2 = 501,
	MOUSE_BUTTON_3 = 502,
	MOUSE_BUTTON_4 = 503,
	MOUSE_BUTTON_5 = 504,
	MOUSE_BUTTON_6 = 505,
	MOUSE_BUTTON_7 = 506,
	MOUSE_BUTTON_8 = 507,
	MOUSE_BUTTON_LAST = MOUSE_BUTTON_8,
	MOUSE_BUTTON_LEFT = MOUSE_BUTTON_1,
	MOUSE_BUTTON_RIGHT = MOUSE_BUTTON_2,
	MOUSE_BUTTON_MIDDLE = MOUSE_BUTTON_3,

	GAMEPAD_BUTTON_A = 600,
	GAMEPAD_BUTTON_B = 601,
	GAMEPAD_BUTTON_X = 602,
	GAMEPAD_BUTTON_Y = 603,
	GAMEPAD_BUTTON_LEFT_BUMPER = 604,
	GAMEPAD_BUTTON_RIGHT_BUMPER = 605,
	GAMEPAD_BUTTON_BACK = 606,
	GAMEPAD_BUTTON_START = 607,
	GAMEPAD_BUTTON_GUIDE = 608,
	GAMEPAD_BUTTON_LEFT_THUMB = 609,
	GAMEPAD_BUTTON_RIGHT_THUMB = 610,
	GAMEPAD_BUTTON_DPAD_UP = 611,
	GAMEPAD_BUTTON_DPAD_RIGHT = 612,
	GAMEPAD_BUTTON_DPAD_DOWN = 613,
	GAMEPAD_BUTTON_DPAD_LEFT = 614,
	GAMEPAD_BUTTON_CROSS = GAMEPAD_BUTTON_A,
	GAMEPAD_BUTTON_CIRCLE = GAMEPAD_BUTTON_B,
	GAMEPAD_BUTTON_SQUARE = GAMEPAD_BUTTON_X,
	GAMEPAD_BUTTON_TRIANGLE = GAMEPAD_BUTTON_Y,
};

enum Mods
{
	SHIFT = 0x0001,
	CONTROL = 0x0002,
	ALT = 0x0004,
	SUPER = 0x0008,
	CAPS_LOCK = 0x0010,
	NUM_LOCK = 0x0020
};

enum class PadAxis
{
	LEFT_X = 0,
	LEFT_Y = 1,
	RIGHT_X = 2,
	RIGHT_Y = 3,
	LEFT_TRIGGER = 4,
	RIGHT_TRIGGER = 5,
};

enum class CursorMode
{
	Default = 0,
	Hidden,
	Disabled,
};

struct JoyState
{
	std::string name;
	s32 id = 0;
	std::vector<f32> axes;
	std::vector<u8> buttons;
};

struct GamepadState
{
	JoyState joyState;
	std::string name;
	s32 id = 0;

	f32 getAxis(PadAxis axis) const;
	bool isPressed(Key button) const;
};

namespace stdfs = std::filesystem;

using OnText = Delegate<char>;
using OnInput = Delegate<Key, Action, Mods>;
using OnMouse = Delegate<f64, f64>;
using OnFocus = Delegate<bool>;
using OnFiledrop = Delegate<stdfs::path const&>;
using OnWindowResize = Delegate<s32, s32>;
using OnClosed = Delegate<>;
} // namespace le
