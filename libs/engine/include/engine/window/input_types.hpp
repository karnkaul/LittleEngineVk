#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "core/std_types.hpp"
#include "core/delegate.hpp"

namespace le
{
enum class Action : u8
{
	eRelease = 0,
	ePress = 1,
	eRepeat = 2,
};

enum class Key
{
	eUnknown = -1,
	eSpace = 32,
	eApostrophe = 39,
	eComma = 44,
	eMinus = 45,
	eFullstop = 46,
	eSlash = 47,
	e0 = 48,
	e1 = 49,
	e2 = 50,
	e3 = 51,
	e4 = 52,
	e5 = 53,
	e6 = 54,
	e7 = 55,
	e8 = 56,
	e9 = 57,
	eSemicolon = 59,
	eEqual = 61,
	eA = 65,
	eB = 66,
	eC = 67,
	eD = 68,
	eE = 69,
	eF = 70,
	eG = 71,
	eH = 72,
	eI = 73,
	eJ = 74,
	eK = 75,
	eL = 76,
	eM = 77,
	eN = 78,
	eO = 79,
	eP = 80,
	eQ = 81,
	eR = 82,
	eS = 83,
	eT = 84,
	eU = 85,
	eV = 86,
	eW = 87,
	eX = 88,
	eY = 89,
	eZ = 90,
	eLeftBracket = 91,
	eBackslash = 92,
	eRightBracket = 93,
	eGrave = 94,
	eBacktick = eGrave,

	eEscape = 256,
	eEnter = 257,
	eTab = 258,
	eBackspace = 259,
	eInsert = 260,
	eDelete = 261,
	eRight = 262,
	eLeft = 263,
	eDown = 264,
	eUp = 265,
	ePageUp = 266,
	ePageDown = 267,
	eHome = 268,
	eEnd = 269,
	eCapsLock = 280,
	eScrollLock = 281,
	eNumLock = 282,
	ePrintScreen = 283,
	ePause = 284,
	eF1 = 290,
	eF2 = 291,
	eF3 = 292,
	eF4 = 293,
	eF5 = 294,
	eF6 = 295,
	eF7 = 296,
	eF8 = 297,
	eF9 = 298,
	eF10 = 299,
	eF11 = 300,
	eF12 = 301,
	eF13 = 302,
	eF14 = 303,
	eF15 = 304,
	eF16 = 305,
	eF17 = 306,
	eF18 = 307,
	eF19 = 308,
	eF20 = 309,
	eF21 = 310,
	eF22 = 311,
	eF23 = 312,
	eF24 = 313,
	eF25 = 314,
	eKp0 = 320,
	eKp1 = 321,
	eKp2 = 322,
	eKp3 = 323,
	eKp4 = 324,
	eKp5 = 325,
	eKp6 = 326,
	eKp7 = 327,
	eKp8 = 328,
	eKp9 = 329,
	eKpDecimal = 330,
	eKpDivide = 331,
	eKpMultiply = 332,
	eKpSubtract = 333,
	eKpAdd = 334,
	eKpEnter = 335,
	eKpEqual = 336,
	eLeftShift = 340,
	eLeftControl = 341,
	eLeftAlt = 342,
	eLeftSuper = 343,
	eRightShift = 344,
	eRightControl = 345,
	eRightAlt = 346,
	eRightSuper = 347,
	eMenu = 348,

	eMouseButton1 = 500,
	eMouseButton2 = 501,
	eMouseButton3 = 502,
	eMouseButton4 = 503,
	eMouseButton5 = 504,
	eMouseButton6 = 505,
	eMouseButton7 = 506,
	eMouseButton8 = 507,
	eMouseButtonLast = eMouseButton8,
	eMouseButtonLeft = eMouseButton1,
	eMouseButtonRight = eMouseButton2,
	eMouseButtonMiddle = eMouseButton3,

	eGamepadButtonA = 600,
	eGamepadButtonB = 601,
	eGamepadButtonX = 602,
	eGamepadButtonY = 603,
	eGamepadButtonLeftBumper = 604,
	eGamepadButtonRightBumper = 605,
	eGamepadButtonBack = 606,
	eGamepadButtonStart = 607,
	eGamepadButtonGuide = 608,
	eGamepadButtonLeftThumb = 609,
	eGamepadButtonRightThumb = 610,
	eGamepadButtonDpadUp = 611,
	eGamepadButtonDpadRight = 612,
	eGamepadButtonDpadDown = 613,
	eGamepadButtonDpadLeft = 614,
	eGamepadButtonCross = eGamepadButtonA,
	eGamepadButtonCircle = eGamepadButtonB,
	eGamepadButtonSquare = eGamepadButtonX,
	eGamepadButtonTriangle = eGamepadButtonY,
};

enum Mods
{
	eSHIFT = 0x0001,
	eCONTROL = 0x0002,
	eALT = 0x0004,
	eSUPER = 0x0008,
	eCAPS_LOCK = 0x0010,
	eNUM_LOCK = 0x0020
};

enum class PadAxis : u8
{
	eLeftX = 0,
	eLeftY = 1,
	eRightX = 2,
	eRightY = 3,
	eLeftTrigger = 4,
	eRightTrigger = 5,
};

enum class CursorMode : u8
{
	eDefault = 0,
	eHidden,
	eDisabled,
};

struct JoyState
{
	std::string name;
	std::vector<f32> axes;
	std::vector<u8> buttons;
	s32 id = 0;
};

struct GamepadState
{
	JoyState joyState;
	std::string name;
	s32 id = 0;

	f32 axis(PadAxis axis) const;
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
