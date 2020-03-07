#include <memory>
#include <unordered_set>
#if defined(LEVK_USE_GLFW)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/threads.hpp"
#include "core/utils.hpp"
#include "engine/window/window.hpp"
#include "engine/vuk/instance/instance.hpp"
#include "engine/vuk/instance/instance_impl.hpp"
#include "engine/vuk/instance/device.hpp"
#include "engine/vuk/context/swapchain.hpp"
#include "window_impl.hpp"

namespace le
{
namespace
{
WindowID g_nextWindowID = WindowID::Null;
std::unordered_set<Window*> g_registeredWindows;
#if defined(LEVK_USE_GLFW)
bool g_bGLFWvkExtensionsSet = false;
#endif

#if defined(LEVK_USE_GLFW)
void onGLFWError(s32 code, char const* desc)
{
	LOG_E("[{}] GLFW Error! [{}]: {}", Window::s_tName, code, desc);
	return;
}
#endif

bool init()
{
#if defined(LEVK_USE_GLFW)
	glfwSetErrorCallback(&onGLFWError);
	if (glfwInit() != GLFW_TRUE)
	{
		LOG_E("[{}] Could not initialise GLFW!", Window::s_tName);
		return false;
	}
	else if (glfwVulkanSupported() != GLFW_TRUE)
	{
		LOG_E("[{}] Vulkan not supported!", Window::s_tName);
		return false;
	}
	else
	{
		LOG_D("[{}] GLFW initialised successfully", Window::s_tName);
	}
	if (!g_bGLFWvkExtensionsSet)
	{
		g_bGLFWvkExtensionsSet = true;
		u32 glfwExtCount;
		char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		vuk::g_instanceData.extensions.reserve((size_t)glfwExtCount);
		for (u32 i = 0; i < glfwExtCount; ++i)
		{
			vuk::g_instanceData.extensions.push_back(glfwExtensions[i]);
		}
	}
	g_bGLFWInit = true;
#endif
#if defined(LEVK_DEBUG)
	vuk::g_instanceData.bAddValidationLayers = true;
#endif
	return true;
}

void deinit()
{
#if defined(LEVK_USE_GLFW)
	glfwTerminate();
	LOG_D("[{}] GLFW terminated", Window::s_tName);
	g_bGLFWInit = false;
#endif
	return;
}

void registerWindow(Window* pWindow)
{
	g_registeredWindows.insert(pWindow);
	LOG_D("[{}] registered. Active: [{}]", Window::s_tName, g_registeredWindows.size());
	return;
}

void unregisterWindow(Window* pWindow)
{
	if (auto search = g_registeredWindows.find(pWindow); search != g_registeredWindows.end())
	{
		g_registeredWindows.erase(search);
		LOG_D("[{}] deregistered. Active: [{}]", Window::s_tName, g_registeredWindows.size());
	}
	return;
}
} // namespace

class WindowImpl final
{
public:
	struct InputCallbacks
	{
		OnText onText;
		OnInput onInput;
		OnMouse onMouse;
		OnMouse onScroll;
		OnFiledrop onFiledrop;
		OnFocus onFocus;
		OnResize onResize;
		OnClosed onClosed;
	};

	InputCallbacks m_input;
	glm::ivec2 m_size = {};
	std::unique_ptr<NativeWindow> m_uNativeWindow;
	std::unique_ptr<vuk::Swapchain> m_uSwapchain;
	Window* m_pWindow;

	WindowImpl(Window* pWindow) : m_pWindow(pWindow) {}

	~WindowImpl()
	{
		close();
		destroy();
	}

	bool create(Window::Data const& data)
	{
		ASSERT(vuk::g_uInstance.get(), "Instance is null!");
		try
		{
			VkSurfaceKHR surface;
			m_uNativeWindow = std::make_unique<NativeWindow>(data);
#if defined(LEVK_USE_GLFW)
			if (glfwCreateWindowSurface(static_cast<VkInstance>(*vuk::g_uInstance), m_uNativeWindow->m_pWindow, nullptr, &surface) != VK_SUCCESS)
			{
				LOG_E("[{}] Failed to create [{}]", Window::s_tName, utils::tName<vk::SurfaceKHR>());
				m_uNativeWindow.reset();
				return false;
			}
#endif
			vuk::SwapchainData swapchainData{surface, framebufferSize()};
			m_uSwapchain = vuk::g_uInstance->device()->createSwapchain(swapchainData, m_pWindow->m_id);
			if (!m_uSwapchain)
			{
				LOG_E("[{}] Failed to create [{}]", Window::s_tName, vuk::Swapchain::s_tName);
				m_uNativeWindow.reset();
				return false;
			}
#if defined(LEVK_USE_GLFW)
			glfwSetWindowSizeCallback(m_uNativeWindow->m_pWindow, &onResize);
			glfwSetKeyCallback(m_uNativeWindow->m_pWindow, &onKey);
			glfwSetCharCallback(m_uNativeWindow->m_pWindow, &onText);
			glfwSetCursorPosCallback(m_uNativeWindow->m_pWindow, &onMouse);
			glfwSetMouseButtonCallback(m_uNativeWindow->m_pWindow, &onMouseButton);
			glfwSetScrollCallback(m_uNativeWindow->m_pWindow, &onScroll);
			glfwSetDropCallback(m_uNativeWindow->m_pWindow, &onFiledrop);
			glfwSetCursorEnterCallback(m_uNativeWindow->m_pWindow, &onFocus);
			auto const c = m_uNativeWindow->m_initialCentre;
			glfwSetWindowPos(m_uNativeWindow->m_pWindow, c.x, c.y);
			if (data.bCentreCursor)
			{
				auto const size = m_uNativeWindow->m_size;
				glfwSetCursorPos(m_uNativeWindow->m_pWindow, size.x / 2, size.y / 2);
			}
			glfwShowWindow(m_uNativeWindow->m_pWindow);
#endif
			LOG_D("[{}:{}] created", Window::s_tName, m_pWindow->m_id);
			return true;
		}
		catch (std::exception const& e)
		{
			LOG_E("[{}:{}] Failed to create window!\n\t{}", Window::s_tName, m_pWindow->m_id, e.what());
			return false;
		}
		LOG_E("[{}:{}] Failed to create window!", Window::s_tName, m_pWindow->m_id);
		return false;
	}

	bool isOpen() const
	{
#if defined(LEVK_USE_GLFW)
		return m_uNativeWindow && m_uNativeWindow->m_pWindow != nullptr && !glfwWindowShouldClose(m_uNativeWindow->m_pWindow);
#endif
	}

	bool exists() const
	{
#if defined(LEVK_USE_GLFW)
		return m_uNativeWindow && m_uNativeWindow->m_pWindow != nullptr;
#else
		return false;
#endif
	}

	bool isClosing() const
	{
#if defined(LEVK_USE_GLFW)
		return m_uNativeWindow && m_uNativeWindow->m_pWindow != nullptr && glfwWindowShouldClose(m_uNativeWindow->m_pWindow);
#else
		return false;
#endif
	}

	void close()
	{
#if defined(LEVK_USE_GLFW)
		ASSERT(threads::isMainThread(), "Window creation on non-main thread!");
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			glfwSetWindowShouldClose(m_uNativeWindow->m_pWindow, 1);
		}
#endif
		return;
	}

	void destroy()
	{
#if defined(LEVK_USE_GLFW)
		ASSERT(threads::isMainThread(), "Window creation on non-main thread!");
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			m_uSwapchain.reset();
			m_uNativeWindow.reset();
			LOG_D("[{}:{}] closed", Window::s_tName, m_pWindow->m_id);
		}
#endif
		m_size = {};
		return;
	}

	glm::ivec2 framebufferSize()
	{
		glm::ivec2 ret;
#if defined(LEVK_USE_GLFW)
		if (m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			glfwGetFramebufferSize(m_uNativeWindow->m_pWindow, &ret.x, &ret.y);
		}
#endif
		return ret;
	}

	void setCursorMode(CursorMode mode) const
	{
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			s32 val;
			switch (mode)
			{
			case CursorMode::Default:
				val = GLFW_CURSOR_NORMAL;
				break;

			case CursorMode::Hidden:
				val = GLFW_CURSOR_HIDDEN;
				break;

			case CursorMode::Disabled:
				val = GLFW_CURSOR_DISABLED;
				break;

			default:
				val = glfwGetInputMode(m_uNativeWindow->m_pWindow, GLFW_CURSOR);
				break;
			}
			glfwSetInputMode(m_uNativeWindow->m_pWindow, GLFW_CURSOR, val);
		}
#endif
		return;
	}

	CursorMode cursorMode() const
	{
		CursorMode ret = CursorMode::Default;
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			s32 val = glfwGetInputMode(m_uNativeWindow->m_pWindow, GLFW_CURSOR);
			switch (val)
			{
			case GLFW_CURSOR_NORMAL:
				ret = CursorMode::Default;
				break;
			case GLFW_CURSOR_HIDDEN:
				ret = CursorMode::Hidden;
				break;
			case GLFW_CURSOR_DISABLED:
				ret = CursorMode::Disabled;
				break;
			default:
				break;
			}
		}
#endif
		return ret;
	}

	glm::vec2 cursorPos() const
	{
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			f64 x, y;
			glfwGetCursorPos(m_uNativeWindow->m_pWindow, &x, &y);
			auto size = glm::vec2(m_size.x, m_size.y) * 0.5f;
			return {(f32)x - size.x, size.y - (f32)y};
		}
#endif
		return {};
	}

	void setCursorPos(glm::vec2 const& pos)
	{
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			glfwSetCursorPos(m_uNativeWindow->m_pWindow, pos.x, pos.y);
		}
#endif
		return;
	}

	std::string getClipboard() const
	{
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
		{
			return glfwGetClipboardString(m_uNativeWindow->m_pWindow);
		}
#endif
		return {};
	}

	static JoyState getJoyState(s32 id)
	{
		JoyState ret;
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit && glfwJoystickPresent(id))
		{
			ret.id = id;
			s32 count;
			auto const axes = glfwGetJoystickAxes(id, &count);
			ret.axes.reserve((size_t)count);
			for (s32 idx = 0; idx < count; ++idx)
			{
				ret.axes.push_back(axes[idx]);
			}
			auto const buttons = glfwGetJoystickButtons(id, &count);
			ret.buttons.reserve((size_t)count);
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

	static GamepadState getGamepadState(s32 id)
	{
		GamepadState ret;
#if defined(LEVK_USE_GLFW)
		GLFWgamepadstate glfwState;
		if (threads::isMainThread() && g_bGLFWInit && glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &glfwState))
		{
			ret.name = glfwGetGamepadName(id);
			ret.id = id;
			ret.joyState = getJoyState(id);
		}
#endif
		return ret;
	}

	static f32 triggerToAxis(f32 triggerValue)
	{
#if defined(LEVK_USE_GLFW)
		return (triggerValue + 1.0f) * 0.5f;
#else
		return triggerValue;
#endif
	}

	static size_t joystickAxesCount(s32 id)
	{
#if defined(LEVK_USE_GLFW)
		s32 max;
		glfwGetJoystickAxes(id, &max);
		return size_t(max);
#else
		return 0;
#endif
	}

	static size_t joysticKButtonsCount(s32 id)
	{
#if defined(LEVK_USE_GLFW)
		s32 max;
		glfwGetJoystickButtons(id, &max);
		return size_t(max);
#else
		return 0;
#endif
	}

	static std::string_view toString(s32 key)
	{
#if defined(LEVK_USE_GLFW)
		return glfwGetKeyName(key, 0);
#else
		static const std::string_view blank = "";
		return blank;
#endif
	}

	static void pollEvents()
	{
#if defined(LEVK_USE_GLFW)
		if (threads::isMainThread() && g_bGLFWInit)
		{
			glfwPollEvents();
		}
#endif
		return;
	}

#if defined(LEVK_USE_GLFW)
	static void onResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_size = {width, height};
				pWindow->m_uImpl->m_input.onResize(width, height);
				LOG_D("[{}:{}] Window resized: [{}x{}]", Window::s_tName, pWindow->m_id, width, height);
			}
		}
		return;
	}

	static void onKey(GLFWwindow* pGLFWwindow, s32 key, s32 /*scancode*/, s32 action, s32 mods)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_input.onInput(Key(key), Action(action), Mods(mods));
				// LOGIF_D(action == GLFW_PRESS, "[{}:{}] Key pressed: [{}/{}]", Window::s_tName, pWindow->m_id, (char)key, key);
			}
		}
		return;
	}

	static void onMouse(GLFWwindow* pGLFWwindow, f64 x, f64 y)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_input.onMouse(x, y);
			}
		}
		return;
	}

	static void onMouseButton(GLFWwindow* pGLFWwindow, s32 key, s32 action, s32 mods)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_input.onInput(Key(key + (s32)Key::MOUSE_BUTTON_1), Action(action), Mods(mods));
			}
		}
		return;
	}

	static void onText(GLFWwindow* pGLFWwindow, u32 codepoint)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_input.onText(static_cast<char>(codepoint));
			}
		}
		return;
	}

	static void onScroll(GLFWwindow* pGLFWwindow, f64 dx, f64 dy)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_input.onScroll(dx, dy);
			}
		}
		return;
	}

	static void onFiledrop(GLFWwindow* pGLFWwindow, s32 count, char const** szPaths)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				for (s32 idx = 0; idx < count; ++idx)
				{
					stdfs::path path(szPaths[idx]);
					pWindow->m_uImpl->m_input.onFiledrop(path);
				}
			}
		}
		return;
	}

	static void onFocus(GLFWwindow* pGLFWwindow, s32 entered)
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow->m_uImpl->m_uNativeWindow && pWindow->m_uImpl->m_uNativeWindow->m_pWindow == pGLFWwindow)
			{
				pWindow->m_uImpl->m_input.onFocus(entered != 0);
			}
		}
		return;
	}
#endif
};

Window::Service::Service()
{
	if (!init())
	{
		throw std::runtime_error("Failed to initialise Window Service!");
	}
}

Window::Service::~Service()
{
	deinit();
}

std::string const Window::s_tName = utils::tName<Window>();

Window::Window()
{
	registerWindow(this);
	m_id = ++g_nextWindowID.handle;
	m_uImpl = std::make_unique<WindowImpl>(this);
	LOG_I("[{}:{}] constructed", s_tName, m_id);
}

Window::Window(Window&&) = default;
Window& Window::operator=(Window&&) = default;
Window::~Window()
{
	LOG_I("[{}:{}] destroyed", s_tName, m_id);
	unregisterWindow(this);
}

void Window::pollEvents()
{
	WindowImpl::pollEvents();
	return;
}

WindowID Window::id() const
{
	return m_id;
}

bool Window::isOpen() const
{
	return m_uImpl ? m_uImpl->isOpen() : false;
}

bool Window::isClosing() const
{
	return m_uImpl ? m_uImpl->isClosing() : false;
}

glm::ivec2 Window::framebufferSize() const
{
	return m_uImpl ? m_uImpl->framebufferSize() : glm::ivec2(0);
}

bool Window::create(Data const& data)
{
	return m_uImpl ? m_uImpl->create(data) : false;
}

void Window::close()
{
	if (m_uImpl)
	{
		m_uImpl->close();
	}
	return;
}

void Window::destroy()
{
	if (m_uImpl)
	{
		m_uImpl->destroy();
	}
	return;
}

OnText::Token Window::registerText(OnText::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onText.subscribe(callback) : OnText::Token();
}

OnInput::Token Window::registerInput(OnInput::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onInput.subscribe(callback) : OnInput::Token();
}

OnMouse::Token Window::registerMouse(OnMouse::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onMouse.subscribe(callback) : OnMouse::Token();
}

OnMouse::Token Window::registerScroll(OnMouse::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onScroll.subscribe(callback) : OnMouse::Token();
}

OnFiledrop::Token Window::registerFiledrop(OnFiledrop::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onFiledrop.subscribe(callback) : OnFiledrop::Token();
}

OnFocus::Token Window::registerFocus(OnFocus::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onFocus.subscribe(callback) : OnFocus::Token();
}

OnResize::Token Window::registerResize(OnResize::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onResize.subscribe(callback) : OnResize::Token();
}

OnClosed::Token Window::registerClosed(OnClosed::Callback callback)
{
	return m_uImpl ? m_uImpl->m_input.onClosed.subscribe(callback) : OnClosed::Token();
}

void Window::setCursorMode(CursorMode mode) const
{
	if (m_uImpl)
	{
		m_uImpl->setCursorMode(mode);
	}
	return;
}

CursorMode Window::cursorMode() const
{
	return m_uImpl ? m_uImpl->cursorMode() : CursorMode::Default;
}

glm::vec2 Window::cursorPos() const
{
	return m_uImpl ? m_uImpl->cursorPos() : glm::vec2(0.0f);
}

void Window::setCursorPos(glm::vec2 const& pos) const
{
	if (m_uImpl)
	{
		m_uImpl->setCursorPos(pos);
	}
	return;
}

std::string Window::getClipboard() const
{
	return m_uImpl ? m_uImpl->getClipboard() : std::string();
}

JoyState Window::getJoyState(s32 id)
{
	return WindowImpl::getJoyState(id);
}

GamepadState Window::getGamepadState(s32 id)
{
	return WindowImpl::getGamepadState(id);
}

f32 Window::triggerToAxis(f32 triggerValue)
{
	return WindowImpl::triggerToAxis(triggerValue);
}

size_t Window::joystickAxesCount(s32 id)
{
	return WindowImpl::joystickAxesCount(id);
}

size_t Window::joysticKButtonsCount(s32 id)
{
	return WindowImpl::joysticKButtonsCount(id);
}

std::string_view Window::toString(s32 key)
{
	return WindowImpl::toString(key);
}
} // namespace le
