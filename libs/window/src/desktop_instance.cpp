#if defined(LEVK_DESKTOP)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/log.hpp>
#include <core/static_any.hpp>
#include <vulkan/vulkan.hpp>
#include <window/desktop_instance.hpp>

namespace le::window {
namespace {
struct Cursor {
	StaticAny<> data;
	CursorType type;
};

struct {
	struct {
		EnumArray<CursorType, Cursor> loaded;
		Cursor active;
	} cursors;
	EventQueue events;
	GLFWwindow* pWindow = nullptr;
	bool bInit = false;
} g_state;

LibLogger* g_log = nullptr;

constexpr std::string_view g_name = "Window";

using lvl = dl::level;

Cursor const& cursor(CursorType type) {
	auto& cursor = g_state.cursors.loaded[(std::size_t)type];
	if (type != CursorType::eDefault && !cursor.data.contains<GLFWcursor*>()) {
		s32 gCursor = 0;
		switch (type) {
		default:
		case CursorType::eDefault:
			break;
		case CursorType::eResizeEW:
			gCursor = GLFW_RESIZE_EW_CURSOR;
			break;
		case CursorType::eResizeNS:
			gCursor = GLFW_RESIZE_NS_CURSOR;
			break;
		case CursorType::eResizeNWSE:
			gCursor = GLFW_RESIZE_NWSE_CURSOR;
			break;
		case CursorType::eResizeNESW:
			gCursor = GLFW_RESIZE_NESW_CURSOR;
			break;
		}
		if (gCursor != 0) {
			cursor.data = glfwCreateStandardCursor(gCursor);
		}
	}
	cursor.type = type;
	return cursor;
}

template <typename T, typename U = T, typename F>
glm::tvec2<T> getGlfwValue(F func) {
	if (g_state.bInit && g_state.pWindow) {
		U x, y;
		func(g_state.pWindow, &x, &y);
		return glm::tvec2<T>((T)x, (T)y);
	}
	return glm::tvec2<T>((T)0, (T)0);
}

bool init(LibLogger& logger) {
	g_log = &logger;
	glfwSetErrorCallback([](s32 code, char const* szDesc) { g_log->log(lvl::error, 1, "[{}] GLFW Error! [{}]: {}", g_name, code, szDesc); });
	if (glfwInit() != GLFW_TRUE) {
		g_log->log(lvl::error, 2, "[{}] Could not initialise GLFW!", g_name);
		return false;
	} else if (glfwVulkanSupported() != GLFW_TRUE) {
		g_log->log(lvl::error, 2, "[{}] Vulkan not supported!", g_name);
		return false;
	} else {
		g_log->log(lvl::info, 1, "[{}] GLFW initialised successfully", g_name);
	}
	return g_state.bInit = true;
}

void deinit() {
	for (Cursor& cursor : g_state.cursors.loaded) {
		if (auto pCursor = cursor.data.get<GLFWcursor*>()) {
			glfwDestroyCursor(pCursor);
		}
	}
	glfwTerminate();
	g_state = {};
	g_log->log(lvl::info, 1, "[{}] GLFW terminated", g_name);
}
} // namespace

namespace {
void fillMods(Mods& out_mods, s32 mods) {
	out_mods[Mod::eShift] = mods & GLFW_MOD_SHIFT;
	out_mods[Mod::eControl] = mods & GLFW_MOD_CONTROL;
	out_mods[Mod::eAlt] = mods & GLFW_MOD_ALT;
	out_mods[Mod::eSuper] = mods & GLFW_MOD_SUPER;
	out_mods[Mod::eCapsLock] = mods & GLFW_MOD_CAPS_LOCK;
	out_mods[Mod::eNumLock] = mods & GLFW_MOD_NUM_LOCK;
}

void onFocus(GLFWwindow* pGLFWwindow, s32 entered) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		event.type = Event::Type::eFocus;
		event.payload.bSet = entered != 0;
		g_state.events.m_events.push_back(event);
		g_log->log(lvl::info, 1, "[{}] Window focus {}", g_name, (entered != 0) ? "gained" : "lost");
	}
}

void onWindowResize(GLFWwindow* pGLFWwindow, s32 width, s32 height) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		event.type = Event::Type::eResize;
		event.payload.resize = {glm::ivec2(width, height), false};
		g_state.events.m_events.push_back(event);
		g_log->log(lvl::info, 1, "[{}] Window resized: [{}x{}]", g_name, width, height);
	}
}

void onFramebufferResize(GLFWwindow* pGLFWwindow, s32 width, s32 height) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		event.type = Event::Type::eResize;
		event.payload.resize = {glm::ivec2(width, height), true};
		g_state.events.m_events.push_back(event);
		g_log->log(lvl::info, 1, "[{}] Framebuffer resized: [{}x{}]", g_name, width, height);
	}
}

void onIconify(GLFWwindow* pWindow, int iconified) {
	if (g_state.bInit && g_state.pWindow == pWindow) {
		Event event;
		event.type = Event::Type::eSuspend;
		event.payload.bSet = iconified != 0;
		g_state.events.m_events.push_back(event);
		g_log->log(lvl::info, 1, "[{}] Window {}", g_name, iconified != 0 ? "suspended" : "resumed");
	}
}

void onClose(GLFWwindow* pGLFWwindow) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		event.type = Event::Type::eClose;
		g_state.events.m_events.push_back(event);
		g_log->log(lvl::info, 1, "[{}] Window closed", g_name);
	}
}

void onKey(GLFWwindow* pGLFWwindow, s32 key, s32 /*scancode*/, s32 action, s32 mods) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		Event::Input input;
		input.key = (Key)key;
		input.action = (Action)action;
		fillMods(input.mods, mods);
		event.type = Event::Type::eInput;
		event.payload.input = input;
		g_state.events.m_events.push_back(event);
	}
}

void onMouse(GLFWwindow* pGLFWwindow, f64 x, f64 y) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		Event::Cursor cursor;
		cursor.x = x;
		cursor.y = y;
		cursor.id = 0;
		event.type = Event::Type::eCursor;
		event.payload.cursor = cursor;
		g_state.events.m_events.push_back(event);
	}
}

void onMouseButton(GLFWwindow* pGLFWwindow, s32 key, s32 action, s32 mods) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		Event::Input input;
		input.key = Key(key + (s32)Key::eMouseButton1);
		input.action = (Action)action;
		fillMods(input.mods, mods);
		event.type = Event::Type::eInput;
		event.payload.input = input;
		g_state.events.m_events.push_back(event);
	}
}

void onText(GLFWwindow* pGLFWwindow, u32 codepoint) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		event.type = Event::Type::eText;
		event.payload.text = {static_cast<char>(codepoint)};
		g_state.events.m_events.push_back(event);
	}
}

void onScroll(GLFWwindow* pGLFWwindow, f64 dx, f64 dy) {
	if (g_state.bInit && g_state.pWindow == pGLFWwindow) {
		Event event;
		Event::Cursor cursor;
		cursor.x = dx;
		cursor.y = dy;
		cursor.id = 0;
		event.type = Event::Type::eScroll;
		event.payload.cursor = cursor;
		g_state.events.m_events.push_back(event);
	}
}
} // namespace

DesktopInstance::DesktopInstance(CreateInfo const& info) : IInstance(true) {
	if (g_state.bInit || g_state.pWindow) {
		throw std::runtime_error("Duplicate GLFW instance");
	}
	if (!init(m_log)) {
		throw std::runtime_error("Fatal errorin creating Window Instance");
	}
	m_log.minVerbosity = info.options.verbosity;
	s32 screenCount;
	GLFWmonitor* const* ppScreens = glfwGetMonitors(&screenCount);
	if (screenCount < 1) {
		m_log.log(lvl::error, 2, "[{}] Failed to detect screens!", g_name);
		throw std::runtime_error("Failed to create Window");
	}
	GLFWvidmode const* mode = glfwGetVideoMode(ppScreens[0]);
	if (!mode) {
		m_log.log(lvl::error, 2, "[{}] Failed to detect video mode!", g_name);
		throw std::runtime_error("Failed to create Window");
	}
	std::size_t const screenIdx = info.options.screenID < screenCount ? (std::size_t)info.options.screenID : 0;
	GLFWmonitor* pTarget = ppScreens[screenIdx];
	s32 height = info.config.size.y;
	s32 width = info.config.size.x;
	bool bDecorated = true;
	switch (info.options.style) {
	default:
	case Style::eDecoratedWindow: {
		if (mode->width < width || mode->height < height) {
			m_log.log(lvl::error, 2, "[{}] Window size [{}x{}] too large for default screen! [{}x{}]", g_name, width, height, mode->width, mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		pTarget = nullptr;
		break;
	}
	case Style::eBorderlessWindow: {
		if (mode->width < width || mode->height < height) {
			m_log.log(lvl::error, 2, "[{}] Window size [{}x{}] too large for default screen! [{}x{}]", g_name, width, height, mode->width, mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		bDecorated = false;
		pTarget = nullptr;
		break;
	}
	case Style::eBorderlessFullscreen: {
		height = (u16)mode->height;
		width = (u16)mode->width;
		break;
	}
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	s32 cX = (mode->width - width) / 2;
	s32 cY = (mode->height - height) / 2;
	cX += info.config.centreOffset.x;
	cY -= info.config.centreOffset.y;
	ENSURE(cX >= 0 && cY >= 0 && cX < mode->width && cY < mode->height, "Invalid centre-screen!");
	glfwWindowHint(GLFW_DECORATED, bDecorated ? 1 : 0);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_VISIBLE, false);
	g_state.pWindow = glfwCreateWindow(width, height, info.config.title.data(), pTarget, nullptr);
	if (!g_state.pWindow) {
		throw std::runtime_error("Failed to create Window");
	}
	glfwSetWindowPos(g_state.pWindow, cX, cY);
	glfwSetCursorEnterCallback(g_state.pWindow, &onFocus);
	glfwSetWindowSizeCallback(g_state.pWindow, &onWindowResize);
	glfwSetFramebufferSizeCallback(g_state.pWindow, &onFramebufferResize);
	glfwSetWindowCloseCallback(g_state.pWindow, &onClose);
	glfwSetKeyCallback(g_state.pWindow, &onKey);
	glfwSetCharCallback(g_state.pWindow, &onText);
	glfwSetCursorPosCallback(g_state.pWindow, &onMouse);
	glfwSetMouseButtonCallback(g_state.pWindow, &onMouseButton);
	glfwSetScrollCallback(g_state.pWindow, &onScroll);
	glfwSetWindowIconifyCallback(g_state.pWindow, &onIconify);
	if (info.options.bAutoShow) {
		show();
	}
	Event event;
	event.type = Event::Type::eInit;
	g_state.events.m_events.push_back(event);
}

DesktopInstance::~DesktopInstance() {
	deinit();
}

Span<std::string_view> DesktopInstance::vkInstanceExtensions() const {
	static std::vector<std::string_view> ret;
	if (ret.empty()) {
		u32 glfwExtCount;
		char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		ret.reserve((std::size_t)glfwExtCount);
		for (u32 i = 0; i < glfwExtCount; ++i) {
			ret.push_back(glfwExtensions[i]);
		}
	}
	return ret;
}

bool DesktopInstance::vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const {
	if (g_state.bInit && g_state.pWindow && vkInstance.contains<vk::Instance>() && out_vkSurface.contains<vk::SurfaceKHR>()) {
		auto& out_surface = out_vkSurface.get<vk::SurfaceKHR>();
		auto const& instance = vkInstance.get<vk::Instance>();
		VkSurfaceKHR surface;
		auto result = glfwCreateWindowSurface(instance, g_state.pWindow, nullptr, &surface);
		if (result == VK_SUCCESS) {
			out_surface = surface;
			return true;
		}
	}
	return false;
}

ErasedRef DesktopInstance::nativePtr() const noexcept {
	return g_state.bInit && g_state.pWindow ? g_state.pWindow : ErasedRef();
}

EventQueue DesktopInstance::pollEvents() {
	if (g_state.bInit && g_state.pWindow) {
		glfwPollEvents();
	}
	return std::move(g_state.events);
}

glm::ivec2 DesktopInstance::windowSize() const noexcept {
	return getGlfwValue<s32>(&glfwGetWindowSize);
}

glm::ivec2 DesktopInstance::framebufferSize() const noexcept {
	return getGlfwValue<s32>(&glfwGetFramebufferSize);
}

CursorType DesktopInstance::cursorType() const noexcept {
	return g_state.cursors.active.type;
}

CursorMode DesktopInstance::cursorMode() const noexcept {
	if (g_state.bInit && g_state.pWindow) {
		s32 val = glfwGetInputMode(g_state.pWindow, GLFW_CURSOR);
		switch (val) {
		case GLFW_CURSOR_NORMAL:
			return CursorMode::eDefault;
		case GLFW_CURSOR_HIDDEN:
			return CursorMode::eHidden;
		case GLFW_CURSOR_DISABLED:
			return CursorMode::eDisabled;
		default:
			break;
		}
	}
	return CursorMode::eDefault;
}

glm::vec2 DesktopInstance::cursorPosition() const noexcept {
	return getGlfwValue<f32, f64>(&glfwGetCursorPos);
}

void DesktopInstance::cursorType(CursorType type) {
	if (g_state.bInit && g_state.pWindow) {
		if (type != g_state.cursors.active.type) {
			g_state.cursors.active = cursor(type);
			glfwSetCursor(g_state.pWindow, g_state.cursors.active.data.get<GLFWcursor*>());
		}
	}
}

void DesktopInstance::cursorMode(CursorMode mode) {
	if (g_state.bInit && g_state.pWindow) {
		s32 val;
		switch (mode) {
		case CursorMode::eDefault:
			val = GLFW_CURSOR_NORMAL;
			break;
		case CursorMode::eHidden:
			val = GLFW_CURSOR_HIDDEN;
			break;
		case CursorMode::eDisabled:
			val = GLFW_CURSOR_DISABLED;
			break;
		default:
			mode = CursorMode::eDefault;
			val = glfwGetInputMode(g_state.pWindow, GLFW_CURSOR);
			break;
		}
		glfwSetInputMode(g_state.pWindow, GLFW_CURSOR, val);
	}
}

void DesktopInstance::cursorPosition(glm::vec2 position) {
	if (g_state.bInit && g_state.pWindow) {
		glfwSetCursorPos(g_state.pWindow, position.x, position.y);
	}
}

void DesktopInstance::show() const {
	if (g_state.bInit && g_state.pWindow) {
		glfwShowWindow(g_state.pWindow);
	}
}

void DesktopInstance::close() {
	if (g_state.bInit && g_state.pWindow) {
		glfwSetWindowShouldClose(g_state.pWindow, 1);
		Event event;
		event.type = Event::Type::eClose;
		g_state.events.m_events.push_back(event);
	}
}

bool DesktopInstance::importControllerDB(std::string_view db) const {
	if (g_state.bInit) {
		glfwUpdateGamepadMappings(db.data());
		return true;
	}
	return false;
}

std::vector<Gamepad> DesktopInstance::activeGamepads() const {
	std::vector<Gamepad> ret;
	for (s32 id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id) {
		GLFWgamepadstate state;
		if (glfwJoystickPresent(id) && glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &state)) {
			Gamepad padi;
			padi.name = glfwGetGamepadName(id);
			padi.id = id;
			padi.buttons = std::vector<u8>(15, 0);
			padi.axes = std::vector<f32>(6, 0);
			std::memcpy(padi.buttons.data(), state.buttons, padi.buttons.size());
			std::memcpy(padi.axes.data(), state.axes, padi.axes.size() * sizeof(f32));
			ret.push_back(std::move(padi));
		}
	}
	return ret;
}

Joystick DesktopInstance::joyState(s32 id) const {
	Joystick ret;
	if (glfwJoystickPresent(id)) {
		ret.id = id;
		s32 count;
		auto const axes = glfwGetJoystickAxes(id, &count);
		ret.axes.reserve((std::size_t)count);
		for (s32 idx = 0; idx < count; ++idx) {
			ret.axes.push_back(axes[idx]);
		}
		auto const buttons = glfwGetJoystickButtons(id, &count);
		ret.buttons.reserve((std::size_t)count);
		for (s32 idx = 0; idx < count; ++idx) {
			ret.buttons.push_back(buttons[idx]);
		}
		auto const szName = glfwGetJoystickName(id);
		if (szName) {
			ret.name = szName;
		}
	}
	return ret;
}

Gamepad DesktopInstance::gamepadState(s32 id) const {
	Gamepad ret;
	GLFWgamepadstate state;
	if (glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &state)) {
		ret.name = glfwGetGamepadName(id);
		ret.id = id;
		ret.buttons = std::vector<u8>(15, 0);
		ret.axes = std::vector<f32>(6, 0);
		std::memcpy(ret.buttons.data(), state.buttons, ret.buttons.size());
		std::memcpy(ret.axes.data(), state.axes, ret.axes.size() * sizeof(f32));
	}
	return ret;
}

f32 DesktopInstance::triggerToAxis(f32 triggerValue) const {
	return (triggerValue + 1.0f) * 0.5f;
}

std::size_t DesktopInstance::joystickAxesCount(s32 id) const {
	s32 max;
	glfwGetJoystickAxes(id, &max);
	return std::size_t(max);
}

std::size_t DesktopInstance::joysticKButtonsCount(s32 id) const {
	s32 max;
	glfwGetJoystickButtons(id, &max);
	return std::size_t(max);
}

std::string_view DesktopInstance::toString(s32 key) const {
	return glfwGetKeyName(key, 0);
}
} // namespace le::window
#endif