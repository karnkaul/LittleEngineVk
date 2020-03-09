#include <array>
#include <unordered_set>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/threads.hpp"
#include "core/utils.hpp"
#include "engine/vuk/context/swapchain.hpp"
#include "window_impl.hpp"
#include "vuk/instance/instance_impl.hpp"

namespace le
{
namespace
{
std::unordered_set<WindowImpl*> g_registeredWindows;
#if defined(LEVK_USE_GLFW)
bool g_bGLFWInit = false;
bool g_bGLFWvkExtensionsSet = false;

void onGLFWError(s32 code, char const* desc)
{
	LOG_E("[{}] GLFW Error! [{}]: {}", Window::s_tName, code, desc);
	return;
}
void onWindowResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_size = {width, height};
			pWindow->m_input.onWindowResize(width, height);
			LOG_D("[{}:{}] Window resized: [{}x{}]", Window::s_tName, pWindow->m_pWindow->id(), width, height);
		}
	}
	return;
}

void onFramebufferResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->onFramebufferSize({width, height});
			LOG_D("[{}:{}] Framebuffer resized: [{}x{}]", Window::s_tName, pWindow->m_pWindow->id(), width, height);
		}
	}
	return;
}

void onKey(GLFWwindow* pGLFWwindow, s32 key, s32 /*scancode*/, s32 action, s32 mods)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_input.onInput(Key(key), Action(action), Mods(mods));
			// LOGIF_D(action == GLFW_PRESS, "[{}:{}] Key pressed: [{}/{}]", Window::s_tName, pWindow->id(), (char)key, key);
		}
	}
	return;
}

void onMouse(GLFWwindow* pGLFWwindow, f64 x, f64 y)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_input.onMouse(x, y);
		}
	}
	return;
}

void onMouseButton(GLFWwindow* pGLFWwindow, s32 key, s32 action, s32 mods)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_input.onInput(Key(key + (s32)Key::eMouseButton1), Action(action), Mods(mods));
		}
	}
	return;
}

void onText(GLFWwindow* pGLFWwindow, u32 codepoint)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_input.onText(static_cast<char>(codepoint));
		}
	}
	return;
}

void onScroll(GLFWwindow* pGLFWwindow, f64 dx, f64 dy)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_input.onScroll(dx, dy);
		}
	}
	return;
}

void onFiledrop(GLFWwindow* pGLFWwindow, s32 count, char const** szPaths)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			for (s32 idx = 0; idx < count; ++idx)
			{
				stdfs::path path(szPaths[idx]);
				pWindow->m_input.onFiledrop(path);
			}
		}
	}
	return;
}

void onFocus(GLFWwindow* pGLFWwindow, s32 entered)
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow)
		{
			pWindow->m_input.onFocus(entered != 0);
		}
	}
	return;
}
#endif

void registerWindow(WindowImpl* pWindow)
{
	g_registeredWindows.insert(pWindow);
	LOG_D("[{}] registered. Active: [{}]", Window::s_tName, g_registeredWindows.size());
	return;
}

void unregisterWindow(WindowImpl* pWindow)
{
	if (auto search = g_registeredWindows.find(pWindow); search != g_registeredWindows.end())
	{
		g_registeredWindows.erase(search);
		LOG_D("[{}] deregistered. Active: [{}]", Window::s_tName, g_registeredWindows.size());
	}
	return;
}

} // namespace

NativeWindow::NativeWindow(Window::Data const& data)
{
#if defined(LEVK_USE_GLFW)
	ASSERT(threads::isMainThread(), "Window creation on non-main thread!");
	if (!threads::isMainThread())
	{
		LOG_E("[{}] Cannot create GLFW window on non-main thread!", Window::s_tName);
		throw std::runtime_error("Failed to create Window");
	}
	s32 screenCount;
	GLFWmonitor** ppScreens = glfwGetMonitors(&screenCount);
	if (screenCount < 1)
	{
		LOG_E("[{}] Failed to detect screens!", Window::s_tName);
		throw std::runtime_error("Failed to create Window");
	}
	GLFWvidmode const* mode = glfwGetVideoMode(ppScreens[0]);
	if (!mode)
	{
		LOG_E("[{}] Failed to detect video mode!", Window::s_tName);
		throw std::runtime_error("Failed to create Window");
	}
	size_t screenIdx = data.screenID < screenCount ? (size_t)data.screenID : 0;
	GLFWmonitor* pTarget = ppScreens[screenIdx];
	s32 height = data.size.y;
	s32 width = data.size.x;
	bool bDecorated = true;
	switch (data.mode)
	{
	default:
	case Window::Mode::eDecoratedWindow:
	{
		if (mode->width < width || mode->height < height)
		{
			LOG_E("[{}] Window size [{}x{}] too large for default screen! [{}x{}]", Window::s_tName, width, height, mode->width,
				  mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		pTarget = nullptr;
		break;
	}
	case Window::Mode::eBorderlessWindow:
	{
		if (mode->width < width || mode->height < height)
		{
			LOG_E("[{}] Window size [{}x{}] too large for default screen! [{}x{}]", Window::s_tName, width, height, mode->width,
				  mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		bDecorated = false;
		pTarget = nullptr;
		break;
	}
	case Window::Mode::eBorderlessFullscreen:
	{
		height = (u16)mode->height;
		width = (u16)mode->width;
		break;
	}
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_size = {width, height};
	s32 cX = (mode->width - width) / 2;
	s32 cY = (mode->height - height) / 2;
	cX += data.position.x;
	cY -= data.position.y;
	m_initialCentre = {cX, cY};
	ASSERT(cX >= 0 && cY >= 0 && cX < mode->width && cY < mode->height, "Invalid centre-screen!");
	glfwWindowHint(GLFW_DECORATED, bDecorated ? 1 : 0);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_VISIBLE, false);
	m_pWindow = glfwCreateWindow(data.size.x, data.size.y, data.title.data(), pTarget, nullptr);
	if (!m_pWindow)
	{
		throw std::runtime_error("Failed to create Window");
	}
#endif
}

NativeWindow::~NativeWindow()
{
#if defined(LEVK_USE_GLFW)
	if (m_pWindow)
	{
		glfwDestroyWindow(m_pWindow);
	}
#endif
}

bool WindowImpl::init()
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

void WindowImpl::deinit()
{
#if defined(LEVK_USE_GLFW)
	glfwTerminate();
	LOG_D("[{}] GLFW terminated", Window::s_tName);
	g_bGLFWInit = false;
#endif
	return;
}

WindowImpl::WindowImpl(Window* pWindow) : m_pWindow(pWindow)
{
	registerWindow(this);
}

WindowImpl::~WindowImpl()
{
	unregisterWindow(this);
	close();
	destroy();
}

bool WindowImpl::create(Window::Data const& data)
{
	ASSERT(vuk::g_uInstance.get() && vuk::g_pDevice, "Instance/Device is null!");
	auto reset = [&]() {
		vuk::g_uInstance->destroy(m_surface);
		m_uNativeWindow.reset();
	};
	try
	{
		m_uNativeWindow = std::make_unique<NativeWindow>(data);
		m_surface = generateSurface(vuk::g_uInstance.get(), *m_uNativeWindow);
		if (m_surface == vk::SurfaceKHR())
		{
			LOG_E("[{}] Failed to create [{}]", Window::s_tName, utils::tName<vk::SurfaceKHR>());
			reset();
			return false;
		}
		vuk::SwapchainData swapchainData{m_surface, framebufferSize()};
		m_uSwapchain = vuk::g_pDevice->createSwapchain(swapchainData, m_pWindow->m_id);
		if (!m_uSwapchain)
		{
			LOG_E("[{}] Failed to create [{}]", Window::s_tName, vuk::Swapchain::s_tName);
			reset();
			return false;
		}
#if defined(LEVK_USE_GLFW)
		glfwSetWindowSizeCallback(m_uNativeWindow->m_pWindow, &onWindowResize);
		glfwSetFramebufferSizeCallback(m_uNativeWindow->m_pWindow, &onFramebufferResize);
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
		reset();
		return false;
	}
	LOG_E("[{}:{}] Failed to create window!", Window::s_tName, m_pWindow->m_id);
	return false;
}

bool WindowImpl::isOpen() const
{
	bool bRet = false;
#if defined(LEVK_USE_GLFW)
	bRet = m_uNativeWindow && m_uNativeWindow->m_pWindow != nullptr && !glfwWindowShouldClose(m_uNativeWindow->m_pWindow);
#endif
	return bRet;
}

bool WindowImpl::exists() const
{
	bool bRet = false;
#if defined(LEVK_USE_GLFW)
	bRet = m_uNativeWindow && m_uNativeWindow->m_pWindow != nullptr;
#endif
	return bRet;
}

bool WindowImpl::isClosing() const
{
	bool bRet = false;
#if defined(LEVK_USE_GLFW)
	bRet = m_uNativeWindow && m_uNativeWindow->m_pWindow != nullptr && glfwWindowShouldClose(m_uNativeWindow->m_pWindow);
#endif
	return bRet;
}

void WindowImpl::close()
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

void WindowImpl::destroy()
{
#if defined(LEVK_USE_GLFW)
	ASSERT(threads::isMainThread(), "Window creation on non-main thread!");
	if (threads::isMainThread() && g_bGLFWInit)
	{
#endif
		if (m_uNativeWindow)
		{
			m_uSwapchain.reset();
			vuk::g_uInstance->destroy(m_surface);
			m_uNativeWindow.reset();
			LOG_D("[{}:{}] closed", Window::s_tName, m_pWindow->m_id);
		}
		m_size = {};
#if defined(LEVK_USE_GLFW)
	}
#endif
	return;
}

vk::SurfaceKHR WindowImpl::generateSurface(vuk::Instance const* pInstance, NativeWindow const& nativeWindow)
{
	ASSERT(pInstance, "Instance is null!");
	vk::SurfaceKHR ret;
#if defined(LEVK_USE_GLFW)
	VkSurfaceKHR surface;
	auto result = glfwCreateWindowSurface(static_cast<VkInstance>(*pInstance), nativeWindow.m_pWindow, nullptr, &surface);
	ASSERT(result == VK_SUCCESS, "Surface creation failed!");
	if (result == VK_SUCCESS)
	{
		ret = surface;
	}
#endif
	return ret;
}

glm::ivec2 WindowImpl::framebufferSize()
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

void WindowImpl::onFramebufferSize(glm::ivec2 const& size)
{
	auto oldSurface = m_surface;
	m_surface = generateSurface(vuk::g_uInstance.get(), *m_uNativeWindow);
	[[maybe_unused]] bool bValid = vuk::g_pDevice->validateSurface(m_surface);
	ASSERT(bValid, "Invalid surface");
	vuk::SwapchainData data{m_surface, size};
	try
	{
		m_uSwapchain->recreate(data);
	}
	catch (std::exception const& e)
	{
		m_uSwapchain->destroy();
		LOG_E("[{}:{}] Failed to recreate [{}]!\n\t{}", Window::s_tName, m_pWindow->m_id, vuk::Swapchain::s_tName, e.what());
	}
	vuk::g_uInstance->destroy(oldSurface);
	return;
}

void WindowImpl::setCursorMode(CursorMode mode) const
{
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
	{
		s32 val;
		switch (mode)
		{
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
			val = glfwGetInputMode(m_uNativeWindow->m_pWindow, GLFW_CURSOR);
			break;
		}
		glfwSetInputMode(m_uNativeWindow->m_pWindow, GLFW_CURSOR, val);
	}
#endif
	return;
}

CursorMode WindowImpl::cursorMode() const
{
	CursorMode ret = CursorMode::eDefault;
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
	{
		s32 val = glfwGetInputMode(m_uNativeWindow->m_pWindow, GLFW_CURSOR);
		switch (val)
		{
		case GLFW_CURSOR_NORMAL:
			ret = CursorMode::eDefault;
			break;
		case GLFW_CURSOR_HIDDEN:
			ret = CursorMode::eHidden;
			break;
		case GLFW_CURSOR_DISABLED:
			ret = CursorMode::eDisabled;
			break;
		default:
			break;
		}
	}
#endif
	return ret;
}

glm::vec2 WindowImpl::cursorPos() const
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

void WindowImpl::setCursorPos(glm::vec2 const& pos)
{
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
	{
		glfwSetCursorPos(m_uNativeWindow->m_pWindow, pos.x, pos.y);
	}
#endif
	return;
}

std::string WindowImpl::getClipboard() const
{
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
	{
		return glfwGetClipboardString(m_uNativeWindow->m_pWindow);
	}
#endif
	return {};
}

JoyState WindowImpl::getJoyState(s32 id)
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

GamepadState WindowImpl::getGamepadState(s32 id)
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

f32 WindowImpl::triggerToAxis(f32 triggerValue)
{
	f32 ret = triggerValue;
#if defined(LEVK_USE_GLFW)
	ret = (triggerValue + 1.0f) * 0.5f;
#endif
	return ret;
}

size_t WindowImpl::joystickAxesCount(s32 id)
{
	size_t ret = 0;
#if defined(LEVK_USE_GLFW)
	s32 max;
	glfwGetJoystickAxes(id, &max);
	ret = size_t(max);
#endif
	return ret;
}

size_t WindowImpl::joysticKButtonsCount(s32 id)
{
	size_t ret = 0;
#if defined(LEVK_USE_GLFW)
	s32 max;
	glfwGetJoystickButtons(id, &max);
	ret = size_t(max);
#endif
	return ret;
}

std::string_view WindowImpl::toString(s32 key)
{
	static const std::string_view blank = "";
	std::string_view ret = blank;
#if defined(LEVK_USE_GLFW)
	ret = glfwGetKeyName(key, 0);
#endif
	return ret;
}

void WindowImpl::pollEvents()
{
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit)
	{
		glfwPollEvents();
	}
#endif
	return;
}
} // namespace le
