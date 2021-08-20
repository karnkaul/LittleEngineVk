#include <unordered_map>
#include <core/not_null.hpp>
#include <instance_impl.hpp>
#include <window/instance.hpp>

#if defined(LEVK_USE_GLFW)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/array_map.hpp>
#include <core/ensure.hpp>
#include <core/log.hpp>
#include <ktl/fixed_any.hpp>
#endif

namespace le::window {
namespace {
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

std::unordered_map<std::string_view, Mod> const g_modsMap = {{"ctrl", Mod::eControl}, {"alt", Mod::eAlt}, {"shift", Mod::eShift}};

std::unordered_map<std::string_view, Axis> const g_axisMap = {
	{"gamepad_left_x", Axis::eLeftX},	{"gamepad_left_y", Axis::eLeftY},	{"gamepad_right_x", Axis::eRightX},
	{"gamepad_right_y", Axis::eRightY}, {"gamepad_lt", Axis::eLeftTrigger}, {"gamepad_rt", Axis::eRightTrigger},
};

template <typename T>
T parse(std::unordered_map<std::string_view, T> const& map, std::string_view str, T fail) {
	if (auto search = map.find(str); search != map.end() && !str.empty()) { return search->second; }
	return fail;
}
} // namespace

using lvl = dl::level;
constexpr std::string_view g_name = "Window";

Manager::Manager() {
#if defined(LEVK_USE_GLFW)
	m_impl = std::make_unique<Impl>(&m_logger);
	if (glfwInit() != GLFW_TRUE) {
		m_logger.log(lvl::error, 2, "[{}] Could not initialise GLFW!", g_name);
		m_impl.reset();
		return;
	} else if (glfwVulkanSupported() != GLFW_TRUE) {
		m_logger.log(lvl::error, 2, "[{}] Vulkan not supported!", g_name);
		m_impl.reset();
		return;
	} else {
		m_logger.log(lvl::info, 1, "[{}] Manager initialised successfully", g_name);
	}
#endif
}

Manager::~Manager() {
#if defined(LEVK_USE_GLFW)
	if (m_impl) {
		for (Cursor& cursor : m_impl->m_cursors.arr) {
			if (cursor.data.contains<GLFWcursor*>()) { glfwDestroyCursor(cursor.data.get<GLFWcursor*>()); }
		}
		glfwTerminate();
		m_logger.log(lvl::info, 1, "[{}] Manager terminated", g_name);
	}
#endif
}

std::optional<Instance> Manager::make(CreateInfo const& info) {
	std::optional<Instance> ret;
#if defined(LEVK_USE_GLFW)
	if (m_impl) {
		if (auto win = m_impl->make(info)) {
			ret = Instance(std::make_unique<Instance::Impl>(m_impl.get(), win));
			if (info.options.autoShow) { ret->show(); }
		}
	}
#endif
	return ret;
}

std::size_t Manager::displayCount() const {
	std::size_t ret{};
#if defined(LEVK_USE_GLFW)
	if (m_impl) { ret = m_impl->displays().size(); }
#endif
	return ret;
}

Instance::Instance(Instance&&) = default;
Instance& Instance::operator=(Instance&&) = default;
Instance::Instance(std::unique_ptr<Impl>&& impl) noexcept : m_impl(std::move(impl)) {}
Instance::~Instance() = default;
EventQueue Instance::pollEvents() { return m_impl->pollEvents(); }
bool Instance::show() { return m_impl->show(); }
bool Instance::hide() { return m_impl->hide(); }
bool Instance::visible() const noexcept { return m_impl->visible(); }
CursorType Instance::cursorType() const noexcept { return m_impl->m_active.type; }
CursorMode Instance::cursorMode() const noexcept { return m_impl->cursorMode(); }
void Instance::cursorType(CursorType type) { m_impl->cursorType(type); }
void Instance::cursorMode(CursorMode mode) { m_impl->cursorMode(mode); }
glm::vec2 Instance::cursorPosition() const noexcept { return m_impl->cursorPosition(); }
glm::uvec2 Instance::windowSize() const noexcept { return m_impl->windowSize(); }
glm::uvec2 Instance::framebufferSize() const noexcept { return m_impl->framebufferSize(); }
void Instance::close() { m_impl->close(); }
bool Instance::closing() const noexcept { return m_impl->closing(); }
bool Instance::importControllerDB(std::string_view db) const { return m_impl->importControllerDB(db); }
ktl::fixed_vector<Gamepad, 8> Instance::activeGamepads() const { return m_impl->activeGamepads(); }
Joystick Instance::joyState(s32 id) const { return m_impl->joyState(id); }
Gamepad Instance::gamepadState(s32 id) const { return m_impl->gamepadState(id); }
std::size_t Instance::joystickAxesCount(s32 id) const { return m_impl->joystickAxesCount(id); }
std::size_t Instance::joysticKButtonsCount(s32 id) const { return m_impl->joysticKButtonsCount(id); }

Key Instance::parseKey(std::string_view str) noexcept { return parse(g_keyMap, str, Key::eUnknown); }

Action Instance::parseAction(std::string_view str) noexcept {
	if (str == "press") { return Action::ePress; }
	return Action::eRelease;
}

Mods Instance::parseMods(Span<std::string const> vec) noexcept {
	Mods ret;
	std::memset(&ret, 0, sizeof(ret));
	for (auto const& str : vec) {
		if (auto search = g_modsMap.find(str); search != g_modsMap.end()) { ret.update(search->second); }
	}
	return ret;
}

Axis Instance::parseAxis(std::string_view str) noexcept { return parse(g_axisMap, str, Axis::eUnknown); }
} // namespace le::window
