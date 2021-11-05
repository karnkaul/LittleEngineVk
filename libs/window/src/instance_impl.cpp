#include <core/array_map.hpp>
#include <core/utils/data_store.hpp>
#include <core/utils/error.hpp>
#include <core/utils/sys_info.hpp>
#include <instance_impl.hpp>

namespace le::window {
using lvl = dl::level;
namespace {
constexpr std::string_view g_name = "Window";

template <typename T, typename U = T, typename F>
glm::tvec2<T> getGlfwValue(GLFWwindow* win, F func) {
	U x, y;
	func(win, &x, &y);
	return glm::tvec2<T>((T)x, (T)y);
}
} // namespace

Manager::Impl::Impl(not_null<LibLogger*> logger) : m_logger(logger) {
	static LibLogger* s_logger{};
	s_logger = logger;
	glfwSetErrorCallback([](s32 code, char const* szDesc) { s_logger->log(lvl::error, 1, "[{}] GLFW Error! [{}]: {}", g_name, code, szDesc); });
}

Manager::Impl::~Impl() { glfwSetErrorCallback(nullptr); }

Cursor const& Manager::Impl::cursor(CursorType type) {
	auto& cursor = m_cursors[type];
	if (type != CursorType::eDefault && !cursor.data.contains<GLFWcursor*>()) {
		s32 gCursor = 0;
		switch (type) {
		default:
		case CursorType::eDefault: break;
		case CursorType::eResizeEW: gCursor = GLFW_RESIZE_EW_CURSOR; break;
		case CursorType::eResizeNS: gCursor = GLFW_RESIZE_NS_CURSOR; break;
		case CursorType::eResizeNWSE: gCursor = GLFW_RESIZE_NWSE_CURSOR; break;
		case CursorType::eResizeNESW: gCursor = GLFW_RESIZE_NESW_CURSOR; break;
		}
		if (gCursor != 0) { cursor.data = glfwCreateStandardCursor(gCursor); }
	}
	cursor.type = type;
	return cursor;
}

GLFWwindow* Manager::Impl::make(CreateInfo const& info) {
	Span<GLFWmonitor* const> screens = displays();
	if (screens.empty()) {
		log().log(lvl::error, 2, "[{}] Failed to detect screens!", g_name);
		throw std::runtime_error("Failed to create Window");
	}
	DataStore::getOrSet<utils::SysInfo>("sys_info").displayCount = screens.size();
	GLFWvidmode const* mode = glfwGetVideoMode(screens[0]);
	if (!mode) {
		log().log(lvl::error, 2, "[{}] Failed to detect video mode!", g_name);
		throw std::runtime_error("Failed to create Window");
	}
	std::size_t const screenIdx = info.options.screenID < screens.size() ? (std::size_t)info.options.screenID : 0;
	GLFWmonitor* target = screens[screenIdx];
	int height = info.config.size.y;
	int width = info.config.size.x;
	bool decorated = true;
	switch (info.options.style) {
	default:
	case Style::eDecoratedWindow: {
		if (mode->width < width || mode->height < height) {
			log().log(lvl::error, 2, "[{}] Window size [{}x{}] too large for default screen! [{}x{}]", g_name, width, height, mode->width, mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		target = nullptr;
		break;
	}
	case Style::eBorderlessWindow: {
		if (mode->width < width || mode->height < height) {
			log().log(lvl::error, 2, "[{}] Window size [{}x{}] too large for default screen! [{}x{}]", g_name, width, height, mode->width, mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		decorated = false;
		target = nullptr;
		break;
	}
	case Style::eBorderlessFullscreen: {
		height = (u16)mode->height;
		width = (u16)mode->width;
		break;
	}
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glm::ivec2 const pos = info.config.position.value_or(glm::ivec2((mode->width - width) / 2, (mode->height - height) / 2));
	glfwWindowHint(GLFW_DECORATED, decorated ? 1 : 0);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_VISIBLE, false);
	glfwWindowHint(GLFW_MAXIMIZED, info.config.maximized);
	auto ret = glfwCreateWindow(width, height, info.config.title.data(), target, nullptr);
	if (ret) { glfwSetWindowPos(ret, pos.x, pos.y); }
	return ret;
}

Span<GLFWmonitor* const> Manager::Impl::displays() const {
	int count;
	GLFWmonitor* const* ppScreens = glfwGetMonitors(&count);
	return Span(ppScreens, static_cast<std::size_t>(count));
}

Instance::Impl::Impl(not_null<Manager::Impl*> manager, not_null<GLFWwindow*> win) : m_win(win), m_manager(manager) {
	glfwSetWindowFocusCallback(m_win, &onFocus);
	glfwSetWindowSizeCallback(m_win, &onWindowResize);
	glfwSetFramebufferSizeCallback(m_win, &onFramebufferResize);
	glfwSetWindowCloseCallback(m_win, &onClose);
	glfwSetKeyCallback(m_win, &onKey);
	glfwSetCharCallback(m_win, &onText);
	glfwSetCursorPosCallback(m_win, &onMouse);
	glfwSetMouseButtonCallback(m_win, &onMouseButton);
	glfwSetScrollCallback(m_win, &onScroll);
	glfwSetWindowIconifyCallback(m_win, &onIconify);
	glfwSetWindowMaximizeCallback(m_win, &onMaximize);
	g_impls.emplace(m_win, this);
}

Instance::Impl::~Impl() {
	glfwDestroyWindow(m_win);
	g_impls.erase(m_win);
}

EventQueue Instance::Impl::pollEvents() {
#if defined(LEVK_USE_GLFW)
	glfwPollEvents();
#endif
	return std::move(m_events);
}

bool Instance::Impl::show() {
	bool ret{};
#if defined(LEVK_USE_GLFW)
	if (!visible()) {
		glfwShowWindow(m_win);
		ret = true;
	}
#endif
	return ret;
}

bool Instance::Impl::hide() {
	bool ret{};
#if defined(LEVK_USE_GLFW)
	if (visible()) {
		glfwHideWindow(m_win);
		ret = true;
	}
#endif
	return ret;
}

bool Instance::Impl::visible() const noexcept {
	bool ret{};
#if defined(LEVK_USE_GLFW)
	ret = glfwGetWindowAttrib(m_win, GLFW_VISIBLE) == 1;
#endif
	return ret;
}

void Instance::Impl::close() {
#if defined(LEVK_USE_GLFW)
	glfwSetWindowShouldClose(m_win, 1);
	Event event;
	event.type = Event::Type::eClose;
	m_events.push_back(event);
#endif
}

bool Instance::Impl::closing() const noexcept {
#if defined(LEVK_USE_GLFW)
	return glfwWindowShouldClose(m_win);
#else
	return false;
#endif
}

glm::ivec2 Instance::Impl::position() const noexcept {
	glm::ivec2 ret{};
#if defined(LEVK_USE_GLFW)
	glfwGetWindowPos(m_win, &ret.x, &ret.y);
#endif
	return ret;
}

void Instance::Impl::position(glm::ivec2 pos) noexcept {
#if defined(LEVK_USE_GLFW)
	glfwSetWindowPos(m_win, pos.x, pos.y);
#endif
}

void Instance::Impl::maximize() noexcept {
#if defined(LEVK_USE_GLFW)
	m_maximized = true;
	glfwMaximizeWindow(m_win);
#endif
}

void Instance::Impl::restore() noexcept {
#if defined(LEVK_USE_GLFW)
	m_maximized = false;
	glfwRestoreWindow(m_win);
#endif
}

CursorMode Instance::Impl::cursorMode() const noexcept {
#if defined(LEVK_USE_GLFW)
	int const val = glfwGetInputMode(m_win, GLFW_CURSOR);
	switch (val) {
	case GLFW_CURSOR_NORMAL: return CursorMode::eDefault;
	case GLFW_CURSOR_HIDDEN: return CursorMode::eHidden;
	case GLFW_CURSOR_DISABLED: return CursorMode::eDisabled;
	default: break;
	}
#endif
	return CursorMode::eDefault;
}

void Instance::Impl::cursorType(CursorType type) {
	if (m_active.type != type) {
#if defined(LEVK_USE_GLFW)
		m_active = m_manager->cursor(type);
		glfwSetCursor(m_win, m_active.data.value_or<GLFWcursor*>(nullptr));
#endif
	}
}

void Instance::Impl::cursorMode([[maybe_unused]] CursorMode mode) {
#if defined(LEVK_USE_GLFW)
	int val;
	switch (mode) {
	case CursorMode::eDefault: val = GLFW_CURSOR_NORMAL; break;
	case CursorMode::eHidden: val = GLFW_CURSOR_HIDDEN; break;
	case CursorMode::eDisabled: val = GLFW_CURSOR_DISABLED; break;
	default:
		mode = CursorMode::eDefault;
		val = glfwGetInputMode(m_win, GLFW_CURSOR);
		break;
	}
	glfwSetInputMode(m_win, GLFW_CURSOR, val);
#endif
}

glm::vec2 Instance::Impl::cursorPosition() const noexcept {
#if defined(LEVK_USE_GLFW)
	return getGlfwValue<f32, f64>(m_win, &glfwGetCursorPos);
#else
	return {};
#endif
}

glm::uvec2 Instance::Impl::windowSize() const noexcept {
#if defined(LEVK_USE_GLFW)
	return getGlfwValue<u32, int>(m_win, &glfwGetWindowSize);
#else
	return {};
#endif
}

glm::uvec2 Instance::Impl::framebufferSize() const noexcept {
#if defined(LEVK_USE_GLFW)
	return getGlfwValue<u32, int>(m_win, &glfwGetFramebufferSize);
#else
	return {};
#endif
}

bool Instance::Impl::importControllerDB(std::string_view db) const {
#if defined(LEVK_USE_GLFW)
	glfwUpdateGamepadMappings(db.data());
	return true;
#else
	return false;
#endif
}

ktl::fixed_vector<Gamepad, 8> Instance::Impl::activeGamepads() const {
	ktl::fixed_vector<Gamepad, 8> ret;
#if defined(LEVK_USE_GLFW)
	for (int id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id) {
		GLFWgamepadstate state;
		if (glfwJoystickPresent(id) && glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &state)) {
			Gamepad padi;
			padi.name = glfwGetGamepadName(id);
			padi.id = (s32)id;
			std::memcpy(padi.buttons.data(), state.buttons, padi.buttons.size());
			std::memcpy(padi.axes.data(), state.axes, padi.axes.size() * sizeof(f32));
			ret.push_back(std::move(padi));
		}
	}
#endif
	return ret;
}

Joystick Instance::Impl::joyState(s32 id) const {
	Joystick ret;
#if defined(LEVK_USE_GLFW)
	if (glfwJoystickPresent((int)id)) {
		ret.id = id;
		int count;
		auto const axes = glfwGetJoystickAxes((int)id, &count);
		ENSURE((std::size_t)count < ret.axes.size(), "Too many axes");
		for (std::size_t idx = 0; idx < (std::size_t)count; ++idx) { ret.axes[idx] = axes[idx]; }
		auto const buttons = glfwGetJoystickButtons((int)id, &count);
		ENSURE((std::size_t)count < ret.buttons.size(), "Too many buttons");
		for (std::size_t idx = 0; idx < (std::size_t)count; ++idx) { ret.buttons[idx] = buttons[idx]; }
		auto const szName = glfwGetJoystickName((int)id);
		if (szName) { ret.name = szName; }
	}
#endif
	return ret;
}

Gamepad Instance::Impl::gamepadState(s32 id) const {
	Gamepad ret;
#if defined(LEVK_USE_GLFW)
	GLFWgamepadstate state;
	if (glfwJoystickIsGamepad((int)id) && glfwGetGamepadState((int)id, &state)) {
		ret.name = glfwGetGamepadName(id);
		ret.id = id;
		std::memcpy(ret.buttons.data(), state.buttons, ret.buttons.size());
		std::memcpy(ret.axes.data(), state.axes, ret.axes.size() * sizeof(f32));
	}
#endif
	return ret;
}

std::size_t Instance::Impl::joystickAxesCount(s32 id) const {
	int max{};
#if defined(LEVK_USE_GLFW)
	glfwGetJoystickAxes((int)id, &max);
#endif
	return std::size_t(max);
}

std::size_t Instance::Impl::joysticKButtonsCount(s32 id) const {
	int max{};
#if defined(LEVK_USE_GLFW)
	glfwGetJoystickButtons((int)id, &max);
#endif
	return std::size_t(max);
}

#if defined(LEVK_USE_GLFW)
void Instance::Impl::onFocus(GLFWwindow* win, int entered) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		event.type = Event::Type::eFocus;
		event.payload.set = entered != 0;
		inst.m_events.push_back(event);
		inst.log().log(lvl::info, 1, "[{}] Window focus {}", g_name, (entered != 0) ? "gained" : "lost");
	}
}

void Instance::Impl::onWindowResize(GLFWwindow* win, int width, int height) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		event.type = Event::Type::eResize;
		event.payload.resize = {glm::ivec2(width, height), false};
		inst.m_events.push_back(event);
		inst.log().log(lvl::debug, 1, "[{}] Window resized: [{}x{}]", g_name, width, height);
	}
}

void Instance::Impl::onFramebufferResize(GLFWwindow* win, int width, int height) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		event.type = Event::Type::eResize;
		event.payload.resize = {glm::ivec2(width, height), true};
		inst.m_events.push_back(event);
		inst.log().log(lvl::debug, 1, "[{}] Framebuffer resized: [{}x{}]", g_name, width, height);
	}
}

void Instance::Impl::onIconify(GLFWwindow* win, int iconified) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		event.type = Event::Type::eSuspend;
		event.payload.set = iconified != 0;
		inst.m_events.push_back(event);
		inst.log().log(lvl::info, 1, "[{}] Window {}", g_name, iconified != 0 ? "suspended" : "resumed");
	}
}

void Instance::Impl::onClose(GLFWwindow* win) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		inst.close();
		inst.log().log(lvl::info, 1, "[{}] Window closed", g_name);
	}
}

void Instance::Impl::onKey(GLFWwindow* win, int key, int scancode, int action, int mods) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		Event::Input input{};
		input.key = (Key)key;
		input.action = (Action)action;
		input.mods = u8(mods);
		input.scancode = scancode;
		event.type = Event::Type::eInput;
		event.payload.input = input;
		inst.m_events.push_back(event);
	}
}

void Instance::Impl::onMouse(GLFWwindow* win, f64 x, f64 y) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		Event::Cursor cursor{};
		cursor.x = x;
		cursor.y = y;
		cursor.id = 0;
		event.type = Event::Type::eCursor;
		event.payload.cursor = cursor;
		inst.m_events.push_back(event);
	}
}

void Instance::Impl::onMouseButton(GLFWwindow* win, int key, int action, int mods) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		Event::Input input{};
		input.key = Key(key + (int)Key::eMouseButton1);
		input.action = (Action)action;
		input.mods = u8(mods);
		input.scancode = 0;
		event.type = Event::Type::eInput;
		event.payload.input = input;
		inst.m_events.push_back(event);
	}
}

void Instance::Impl::onText(GLFWwindow* win, u32 codepoint) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		event.type = Event::Type::eText;
		event.payload.codepoint = codepoint;
		inst.m_events.push_back(event);
	}
}

void Instance::Impl::onScroll(GLFWwindow* win, f64 dx, f64 dy) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		Event event;
		Event::Cursor cursor{};
		cursor.x = dx;
		cursor.y = dy;
		cursor.id = 0;
		event.type = Event::Type::eScroll;
		event.payload.cursor = cursor;
		inst.m_events.push_back(event);
	}
}

void Instance::Impl::onMaximize(GLFWwindow* win, int maximized) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& inst = *it->second;
		inst.m_maximized = maximized != 0;
		Event event;
		event.type = Event::Type::eMaximize;
		event.payload.set = inst.m_maximized;
		inst.m_events.push_back(event);
	}
}
#endif
} // namespace le::window
