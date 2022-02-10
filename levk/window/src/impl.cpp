#include <impl.hpp>
#include <levk/core/array_map.hpp>
#include <levk/core/utils/data_store.hpp>
#include <levk/core/utils/error.hpp>
#include <levk/core/utils/sys_info.hpp>

namespace le::window {
using lvl = LogLevel;
namespace {
constexpr std::string_view g_name = "Window";

template <typename T, typename U = T, typename F>
glm::tvec2<T> getGlfwValue(GLFWwindow* win, F func) {
	U x, y;
	func(win, &x, &y);
	return glm::tvec2<T>((T)x, (T)y);
}
} // namespace

std::optional<GlfwInst> GlfwInst::make() noexcept {
	if (glfwInit() != GLFW_TRUE) {
		logE(LC_EndUser, "[{}] Failed to initialize GLFW!", g_name);
		return std::nullopt;
	}
	if (glfwVulkanSupported() != GLFW_TRUE) {
		logE(LC_EndUser, "[{}] Vulkan not supported!", g_name);
		glfwTerminate();
		return std::nullopt;
	}
	glfwSetErrorCallback([](int code, char const* szDesc) { logE(LC_EndUser, "[{}] GLFW Error! [{}]: {}", g_name, code, szDesc); });
	return GlfwInst{{}, true};
}

void GlfwDel::operator()(GlfwInst const& inst) const noexcept {
	for (Cursor const& cursor : inst.cursors.arr) {
		if (cursor.data.contains<GLFWcursor*>()) { glfwDestroyCursor(cursor.data.get<GLFWcursor*>()); }
	}
	glfwSetErrorCallback(nullptr);
	glfwTerminate();
	log(lvl::info, LC_LibUser, "[{}] Manager terminated", g_name);
}

void GlfwDel::operator()(GLFWwindow* win) const noexcept {
	glfwDestroyWindow(win);
	g_impls.erase(win);
}

Cursor const& Manager::Impl::cursor(CursorType type) {
	auto& cursor = inst->cursors[type];
	if (type != CursorType::eDefault && !cursor.data.contains<GLFWcursor*>()) {
		int gCursor = 0;
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

UniqueGlfwWin Manager::Impl::make(CreateInfo const& info) {
	Span<GLFWmonitor* const> screens = displays();
	DataStore::getOrSet<utils::SysInfo>("sys_info").displayCount = screens.size();
	GLFWvidmode const* mode = screens.empty() ? nullptr : glfwGetVideoMode(screens.front());
	GLFWmonitor* target{};
	auto size = info.config.size;
	int decorated = GLFW_TRUE;
	auto style = info.options.style;
	if ((style == Style::eBorderlessFullscreen || style == Style::eDedicatedFullscreen) && (screens.empty() || !mode)) {
		log(lvl::warn, LC_EndUser, "[{}] Failed to detect monitors/desktop mode; reverting to windowed mode", g_name);
		style = Style::eDecoratedWindow;
	}
	switch (info.options.style) {
	default: break;
	case Style::eBorderlessWindow: decorated = GLFW_FALSE; break;
	case Style::eBorderlessFullscreen: size = {mode->width, mode->height}; break;
	case Style::eDedicatedFullscreen: target = info.options.screenID < screens.size() ? screens[info.options.screenID] : screens[0]; break;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	if (mode) {
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	}
	glfwWindowHint(GLFW_DECORATED, decorated);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, info.config.maximized);
	auto ret = glfwCreateWindow(size.x, size.y, info.config.title.data(), target, nullptr);
	if (!ret) {
		log(lvl::error, LC_EndUser, "[{}] Failed to create window", g_name);
		return {};
	}
	if (style == Style::eDecoratedWindow || style == Style::eBorderlessWindow) {
		glm::ivec2 pos{};
		if (info.config.position) {
			pos = *info.config.position;
		} else {
			auto const datum = mode ? glm::ivec2(mode->width, mode->height) : glm::ivec2();
			pos = datum.x > 0 && datum.y > 0 ? datum - size : glm::ivec2();
			if (pos.x > 0 && pos.y > 0) { pos /= 2; }
		}
		glfwSetWindowPos(ret, pos.x, pos.y);
	}
	if (info.options.autoShow) { glfwShowWindow(ret); }
	return ret;
}

Span<GLFWmonitor* const> Manager::Impl::displays() const {
	int count;
	GLFWmonitor* const* ppScreens = glfwGetMonitors(&count);
	return Span(ppScreens, static_cast<std::size_t>(count));
}

Window::Impl::Impl(not_null<Manager::Impl*> manager, UniqueGlfwWin win) : m_win(std::move(win)), m_manager(manager) {
	if (m_win) {
		glfwSetWindowCloseCallback(*m_win, &onClose);
		glfwSetWindowFocusCallback(*m_win, &onFocus);
		glfwSetCursorEnterCallback(*m_win, &onCursorEnter);
		glfwSetWindowMaximizeCallback(*m_win, &onMaximize);
		glfwSetWindowIconifyCallback(*m_win, &onIconify);
		glfwSetWindowPosCallback(*m_win, &onPosition);
		glfwSetWindowSizeCallback(*m_win, &onWindowResize);
		glfwSetFramebufferSizeCallback(*m_win, &onFramebufferResize);
		glfwSetCursorPosCallback(*m_win, &onCursorPos);
		glfwSetScrollCallback(*m_win, &onScroll);
		glfwSetKeyCallback(*m_win, &onKey);
		glfwSetMouseButtonCallback(*m_win, &onMouseButton);
		glfwSetCharCallback(*m_win, &onText);
		glfwSetDropCallback(*m_win, &onFileDrop);
		g_impls.emplace(*m_win, this);
		m_events.reserve(64);
	}
}

Window::Impl::~Impl() noexcept {
	if (m_win) {
		glfwSetWindowCloseCallback(*m_win, {});
		glfwSetWindowFocusCallback(*m_win, {});
		glfwSetCursorEnterCallback(*m_win, {});
		glfwSetWindowMaximizeCallback(*m_win, {});
		glfwSetWindowIconifyCallback(*m_win, {});
		glfwSetWindowPosCallback(*m_win, {});
		glfwSetWindowSizeCallback(*m_win, {});
		glfwSetFramebufferSizeCallback(*m_win, {});
		glfwSetCursorPosCallback(*m_win, {});
		glfwSetScrollCallback(*m_win, {});
		glfwSetKeyCallback(*m_win, {});
		glfwSetMouseButtonCallback(*m_win, {});
		glfwSetCharCallback(*m_win, {});
		glfwSetDropCallback(*m_win, {});
	}
}

Span<Event const> Window::Impl::pollEvents() {
#if defined(LEVK_USE_GLFW)
	m_events.clear();
	m_eventStorage.drops.clear();
	glfwPollEvents();
#endif
	return m_events;
}

bool Window::Impl::show() {
	bool ret{};
#if defined(LEVK_USE_GLFW)
	if (!visible()) {
		glfwShowWindow(*m_win);
		ret = true;
	}
#endif
	return ret;
}

bool Window::Impl::hide() {
	bool ret{};
#if defined(LEVK_USE_GLFW)
	if (visible()) {
		glfwHideWindow(*m_win);
		ret = true;
	}
#endif
	return ret;
}

bool Window::Impl::visible() const noexcept {
	bool ret{};
#if defined(LEVK_USE_GLFW)
	ret = glfwGetWindowAttrib(*m_win, GLFW_VISIBLE) == 1;
#endif
	return ret;
}

void Window::Impl::close() {
#if defined(LEVK_USE_GLFW)
	glfwSetWindowShouldClose(*m_win, 1);
	m_events.push_back(Event::Builder{}(Event::Type::eClosed, Box{true}));
#endif
}

bool Window::Impl::closing() const noexcept {
#if defined(LEVK_USE_GLFW)
	return glfwWindowShouldClose(*m_win);
#else
	return false;
#endif
}

glm::ivec2 Window::Impl::position() const noexcept {
	glm::ivec2 ret{};
#if defined(LEVK_USE_GLFW)
	glfwGetWindowPos(*m_win, &ret.x, &ret.y);
#endif
	return ret;
}

void Window::Impl::position(glm::ivec2 pos) noexcept {
#if defined(LEVK_USE_GLFW)
	glfwSetWindowPos(*m_win, pos.x, pos.y);
#endif
}

void Window::Impl::maximize() noexcept {
#if defined(LEVK_USE_GLFW)
	m_maximized = true;
	glfwMaximizeWindow(*m_win);
#endif
}

void Window::Impl::restore() noexcept {
#if defined(LEVK_USE_GLFW)
	m_maximized = false;
	glfwRestoreWindow(*m_win);
#endif
}

CursorMode Window::Impl::cursorMode() const noexcept {
#if defined(LEVK_USE_GLFW)
	int const val = glfwGetInputMode(*m_win, GLFW_CURSOR);
	switch (val) {
	case GLFW_CURSOR_NORMAL: return CursorMode::eDefault;
	case GLFW_CURSOR_HIDDEN: return CursorMode::eHidden;
	case GLFW_CURSOR_DISABLED: return CursorMode::eDisabled;
	default: break;
	}
#endif
	return CursorMode::eDefault;
}

void Window::Impl::cursorType(CursorType type) {
	if (m_active.type != type) {
#if defined(LEVK_USE_GLFW)
		m_active = m_manager->cursor(type);
		glfwSetCursor(*m_win, m_active.data.value_or<GLFWcursor*>(nullptr));
#endif
	}
}

void Window::Impl::cursorMode([[maybe_unused]] CursorMode mode) {
#if defined(LEVK_USE_GLFW)
	int val;
	switch (mode) {
	case CursorMode::eDefault: val = GLFW_CURSOR_NORMAL; break;
	case CursorMode::eHidden: val = GLFW_CURSOR_HIDDEN; break;
	case CursorMode::eDisabled: val = GLFW_CURSOR_DISABLED; break;
	default:
		mode = CursorMode::eDefault;
		val = glfwGetInputMode(*m_win, GLFW_CURSOR);
		break;
	}
	glfwSetInputMode(*m_win, GLFW_CURSOR, val);
#endif
}

glm::vec2 Window::Impl::cursorPosition() const noexcept {
#if defined(LEVK_USE_GLFW)
	return getGlfwValue<f32, f64>(*m_win, &glfwGetCursorPos);
#else
	return {};
#endif
}

glm::uvec2 Window::Impl::windowSize() const noexcept {
#if defined(LEVK_USE_GLFW)
	return getGlfwValue<u32, int>(*m_win, &glfwGetWindowSize);
#else
	return {};
#endif
}

glm::uvec2 Window::Impl::framebufferSize() const noexcept {
#if defined(LEVK_USE_GLFW)
	return getGlfwValue<u32, int>(*m_win, &glfwGetFramebufferSize);
#else
	return {};
#endif
}

bool Window::Impl::importControllerDB(std::string_view db) const noexcept {
#if defined(LEVK_USE_GLFW)
	glfwUpdateGamepadMappings(db.data());
	return true;
#else
	return false;
#endif
}

void Window::Impl::setIcon(Span<TBitmap<BmpView> const> bitmaps) {
#if defined(LEVK_USE_GLFW)
	std::vector<GLFWimage> images;
	for (auto const& bitmap : bitmaps) {
		images.push_back(GLFWimage{.width = (int)bitmap.extent.x, .height = (int)bitmap.extent.y, .pixels = const_cast<u8*>(bitmap.bytes.data())});
	}
	glfwSetWindowIcon(*m_win, int(images.size()), images.data());
#endif
}

ktl::fixed_vector<Gamepad, 8> Window::Impl::activeGamepads() const noexcept {
	ktl::fixed_vector<Gamepad, 8> ret;
#if defined(LEVK_USE_GLFW)
	for (int id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id) {
		GLFWgamepadstate state;
		if (glfwJoystickPresent(id) && glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &state)) {
			Gamepad padi;
			padi.id = (int)id;
			std::memcpy(padi.buttons.data(), state.buttons, padi.buttons.size());
			std::memcpy(padi.axes.data(), state.axes, padi.axes.size() * sizeof(f32));
			ret.push_back(std::move(padi));
		}
	}
#endif
	return ret;
}

Joystick Window::Impl::joyState(int id) const noexcept {
	Joystick ret;
#if defined(LEVK_USE_GLFW)
	if (glfwJoystickPresent(id)) {
		ret.id = id;
		int count{};
		auto const axes = glfwGetJoystickAxes((int)id, &count);
		std::size_t end = std::min(std::size_t(count), ret.axes.size());
		for (std::size_t idx = 0; idx < end; ++idx) { ret.axes[idx] = axes[idx]; }
		auto const buttons = glfwGetJoystickButtons((int)id, &count);
		end = std::min(std::size_t(count), ret.buttons.size());
		for (std::size_t idx = 0; idx < end; ++idx) { ret.buttons[idx] = buttons[idx]; }
	}
#endif
	return ret;
}

Gamepad Window::Impl::gamepadState(int id) const noexcept {
	Gamepad ret;
#if defined(LEVK_USE_GLFW)
	GLFWgamepadstate state;
	if (glfwJoystickIsGamepad((int)id) && glfwGetGamepadState((int)id, &state)) {
		ret.id = id;
		std::memcpy(ret.buttons.data(), state.buttons, ret.buttons.size());
		std::memcpy(ret.axes.data(), state.axes, ret.axes.size() * sizeof(f32));
	}
#endif
	return ret;
}

std::size_t Window::Impl::joystickAxesCount(int id) const noexcept {
	int max{};
#if defined(LEVK_USE_GLFW)
	glfwGetJoystickAxes((int)id, &max);
#endif
	return std::size_t(max);
}

std::size_t Window::Impl::joysticKButtonsCount(int id) const noexcept {
	int max{};
#if defined(LEVK_USE_GLFW)
	glfwGetJoystickButtons((int)id, &max);
#endif
	return std::size_t(max);
}

#if defined(LEVK_USE_GLFW)
void Window::Impl::onClose(GLFWwindow* win) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& impl = *it->second;
		impl.close();
		log(lvl::info, LC_LibUser, "[{}] Window closed", g_name);
	}
}

void Window::Impl::onFocus(GLFWwindow* win, int entered) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		it->second->m_events.push_back(Event::Builder{}(Event::Type::eFocusChange, Box{entered == GLFW_TRUE}));
		log(lvl::info, LC_LibUser, "[{}] Window focus {}", g_name, (entered != 0) ? "gained" : "lost");
	}
}

void Window::Impl::onCursorEnter(GLFWwindow* win, int entered) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		it->second->m_events.push_back(Event::Builder{}(Event::Type::eCursorEnter, Box{entered == GLFW_TRUE}));
	}
}

void Window::Impl::onMaximize(GLFWwindow* win, int maximized) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& impl = *it->second;
		impl.m_maximized = maximized != 0;
		impl.m_events.push_back(Event::Builder{}(Event::Type::eMaximize, Box{maximized == GLFW_TRUE}));
	}
}

void Window::Impl::onIconify(GLFWwindow* win, int iconified) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		it->second->m_events.push_back(Event::Builder{}(Event::Type::eIconify, Box{iconified == GLFW_TRUE}));
		log(lvl::info, LC_LibUser, "[{}] Window {}", g_name, iconified != 0 ? "iconified" : "resumed");
	}
}

void Window::Impl::onPosition(GLFWwindow* win, int x, int y) {
	if (auto it = g_impls.find(win); it != g_impls.end()) { it->second->m_events.push_back(Event::Builder{}(Event::Type::ePosition, glm::ivec2{x, y})); }
}

void Window::Impl::onWindowResize(GLFWwindow* win, int width, int height) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		it->second->m_events.push_back(Event::Builder{}(Event::Type::eWindowResize, glm::uvec2{u32(width), u32(height)}));
		log(lvl::debug, LC_LibUser, "[{}] Window resized: [{}x{}]", g_name, width, height);
	}
}

void Window::Impl::onFramebufferResize(GLFWwindow* win, int width, int height) {
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		it->second->m_events.push_back(Event::Builder{}(Event::Type::eFramebufferResize, glm::uvec2{u32(width), u32(height)}));
		log(lvl::debug, LC_LibUser, "[{}] Framebuffer resized: [{}x{}]", g_name, width, height);
	}
}

void Window::Impl::onCursorPos(GLFWwindow* win, f64 x, f64 y) {
	if (auto it = g_impls.find(win); it != g_impls.end()) { it->second->m_events.push_back(Event::Builder{}(Event::Type::eCursor, glm::dvec2{x, y})); }
}

void Window::Impl::onScroll(GLFWwindow* win, f64 dx, f64 dy) {
	if (auto it = g_impls.find(win); it != g_impls.end()) { it->second->m_events.push_back(Event::Builder{}(Event::Type::eScroll, glm::dvec2{dx, dy})); }
}

void Window::Impl::onKey(GLFWwindow* win, int key, int scancode, int action, int mods) {
	if (auto it = g_impls.find(win); it != g_impls.end()) { it->second->m_events.push_back(Event::Builder{}(Event::Key{scancode, key, action, mods})); }
}

void Window::Impl::onMouseButton(GLFWwindow* win, int button, int action, int mods) {
	if (auto it = g_impls.find(win); it != g_impls.end()) { it->second->m_events.push_back(Event::Builder{}(Event::Button{button, action, mods})); }
}

void Window::Impl::onText(GLFWwindow* win, u32 codepoint) {
	if (auto it = g_impls.find(win); it != g_impls.end()) { it->second->m_events.push_back(Event::Builder{}(Box{codepoint})); }
}

void Window::Impl::onFileDrop(GLFWwindow* win, int count, char const** paths) {
	auto const makeDrop = [](int count, char const** paths) {
		EventStorage::Drop ret;
		ret.reserve(std::size_t(count));
		for (int i = 0; i < count; ++i) { ret.push_back(paths[i]); }
		return ret;
	};
	if (auto it = g_impls.find(win); it != g_impls.end()) {
		auto& impl = *it->second;
		impl.m_eventStorage.drops.push_back(makeDrop(count, paths));
		impl.m_events.push_back(Event::Builder{}(impl.m_eventStorage.drops));
	}
}
#endif
} // namespace le::window
