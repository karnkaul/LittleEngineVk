#include <array>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/os.hpp"
#include "core/threads.hpp"
#include "core/utils.hpp"
#include "gfx/common.hpp"
#include "gfx/device.hpp"
#include "gfx/ext_gui.hpp"
#include "gfx/renderer_impl.hpp"
#if defined(LEVK_USE_GLFW)
#if defined(LEVK_RUNTIME_MSVC)
#include <Windows.h>
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
#include "window_impl.hpp"

namespace le
{
namespace
{
std::unordered_set<WindowImpl*> g_registeredWindows;

#if defined(LEVK_USE_GLFW)
bool g_bGLFWInit = false;

WindowImpl* find(GLFWwindow* pGLFWwindow)
{
	auto f = [pGLFWwindow](auto pWindow) -> bool { return pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->m_pWindow == pGLFWwindow; };
	auto search = std::find_if(g_registeredWindows.begin(), g_registeredWindows.end(), f);
	return search != g_registeredWindows.end() ? *search : nullptr;
}

void onGLFWError(s32 code, char const* desc)
{
	LOG_E("[{}] GLFW Error! [{}]: {}", Window::s_tName, code, desc);
	return;
}
void onWindowResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
{
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		pWindow->m_windowSize = {width, height};
		WindowImpl::s_input[pWindow->m_pWindow->id()].onWindowResize(width, height);
		LOG_D("[{}:{}] Window resized: [{}x{}]", Window::s_tName, pWindow->m_pWindow->id(), width, height);
	}
	return;
}

void onFramebufferResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
{
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		pWindow->onFramebufferSize({width, height});
		LOG_D("[{}:{}] Framebuffer resized: [{}x{}]", Window::s_tName, pWindow->m_pWindow->id(), width, height);
	}
	return;
}

void onKey(GLFWwindow* pGLFWwindow, s32 key, s32 /*scancode*/, s32 action, s32 mods)
{
	WindowImpl::s_input[WindowID::s_null].onInput(Key(key), Action(action), Mods(mods));
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onInput(Key(key), Action(action), Mods(mods));
		// LOGIF_D(action == GLFW_PRESS, "[{}:{}] Key pressed: [{}/{}]", Window::s_tName, pWindow->id(), (char)key, key);
	}
	return;
}

void onMouse(GLFWwindow* pGLFWwindow, f64 x, f64 y)
{
	WindowImpl::s_input[WindowID::s_null].onMouse(x, y);
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onMouse(x, y);
	}
	return;
}

void onMouseButton(GLFWwindow* pGLFWwindow, s32 key, s32 action, s32 mods)
{
	WindowImpl::s_input[WindowID::s_null].onInput(Key(key + (s32)Key::eMouseButton1), Action(action), Mods(mods));
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onInput(Key(key + (s32)Key::eMouseButton1), Action(action), Mods(mods));
	}
	return;
}

void onText(GLFWwindow* pGLFWwindow, u32 codepoint)
{
	WindowImpl::s_input[WindowID::s_null].onText(static_cast<char>(codepoint));
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onText(static_cast<char>(codepoint));
	}
	return;
}

void onScroll(GLFWwindow* pGLFWwindow, f64 dx, f64 dy)
{
	WindowImpl::s_input[WindowID::s_null].onScroll(dx, dy);
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onScroll(dx, dy);
	}
	return;
}

void onFiledrop(GLFWwindow* pGLFWwindow, s32 count, char const** szPaths)
{
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		for (size_t idx = 0; idx < (size_t)count; ++idx)
		{
			stdfs::path path(szPaths[idx]);
		}
	}
	return;
}

void onFocus(GLFWwindow* pGLFWwindow, s32 entered)
{
	if (auto pWindow = find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onFocus(entered != 0);
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

f32 GamepadState::axis(PadAxis axis) const
{
	[[maybe_unused]] size_t idx = size_t(axis);
#if defined(LEVK_USE_GLFW)
	s32 max = 0;
	glfwGetJoystickAxes(id, &max);
	if (idx < (size_t)max)
	{
		return joyState.axes.at(idx);
	}
#endif
	return 0.0f;
}

bool GamepadState::isPressed(Key button) const
{
	[[maybe_unused]] size_t idx = (size_t)button - (size_t)Key::eGamepadButtonA;
#if defined(LEVK_USE_GLFW)
	s32 max = 0;
	glfwGetJoystickButtons(id, &max);
	if (idx < (size_t)max)
	{
		return joyState.buttons[idx];
	}
#endif
	return false;
}

NativeWindow::NativeWindow([[maybe_unused]] Window::Info const& info)
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
	size_t screenIdx = info.options.screenID < screenCount ? (size_t)info.options.screenID : 0;
	GLFWmonitor* pTarget = ppScreens[screenIdx];
	s32 height = info.config.size.y;
	s32 width = info.config.size.x;
	bool bDecorated = true;
	switch (info.config.mode)
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
	s32 cX = (mode->width - width) / 2;
	s32 cY = (mode->height - height) / 2;
	cX += info.config.centreOffset.x;
	cY -= info.config.centreOffset.y;
	m_initialCentre = {cX, cY};
	ASSERT(cX >= 0 && cY >= 0 && cX < mode->width && cY < mode->height, "Invalid centre-screen!");
	glfwWindowHint(GLFW_DECORATED, bDecorated ? 1 : 0);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_VISIBLE, false);
	m_pWindow = glfwCreateWindow(width, height, info.config.title.data(), pTarget, nullptr);
	if (!m_pWindow)
	{
		throw std::runtime_error("Failed to create Window");
	}
#else
	throw std::runtime_error("Unsupported configuration");
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

glm::ivec2 NativeWindow::windowSize() const
{
	glm::ivec2 ret = {};
#if defined(LEVK_USE_GLFW)
	if (m_pWindow)
	{
		glfwGetWindowSize(m_pWindow, &ret.x, &ret.y);
	}
#endif
	return ret;
}

glm::ivec2 NativeWindow::framebufferSize() const
{
	glm::ivec2 ret = {};
#if defined(LEVK_USE_GLFW)
	if (m_pWindow)
	{
		glfwGetFramebufferSize(m_pWindow, &ret.x, &ret.y);
	}
#endif
	return ret;
}

std::unordered_map<s32, WindowImpl::InputCallbacks> WindowImpl::s_input;

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
	g_bGLFWInit = true;
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
	s_input.clear();
	return;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
void WindowImpl::pollAssets()
{
	for (auto pWindow : g_registeredWindows)
	{
		if (auto pRenderer = pWindow->m_pWindow->m_renderer.m_uImpl.get())
		{
			pRenderer->pollAssets();
		}
	}
}
#endif

std::vector<char const*> WindowImpl::vulkanInstanceExtensions()
{
	std::vector<char const*> ret;
#if defined(LEVK_USE_GLFW)
	u32 glfwExtCount;
	char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
	ret.reserve((size_t)glfwExtCount);
	for (u32 i = 0; i < glfwExtCount; ++i)
	{
		ret.push_back(glfwExtensions[i]);
	}
#endif
	return ret;
}

WindowImpl* WindowImpl::windowImpl(WindowID window)
{
	auto f = [window](auto pImpl) -> bool { return pImpl->m_pWindow->m_id == window; };
	auto search = std::find_if(g_registeredWindows.begin(), g_registeredWindows.end(), f);
	return search != g_registeredWindows.end() ? *search : nullptr;
}

gfx::RendererImpl* WindowImpl::rendererImpl(WindowID window)
{
	if (auto pImpl = windowImpl(window); pImpl)
	{
		return pImpl->m_pWindow->m_renderer.m_uImpl.get();
	}
	return nullptr;
}

std::unordered_set<s32> WindowImpl::active()
{
	std::unordered_set<s32> ret;
	for (auto pImpl : g_registeredWindows)
	{
		if (pImpl->isOpen())
		{
			ret.insert(pImpl->m_pWindow->m_id);
		}
	}
	return ret;
}

vk::SurfaceKHR WindowImpl::createSurface([[maybe_unused]] vk::Instance instance, [[maybe_unused]] NativeWindow const& nativeWindow)
{
	vk::SurfaceKHR ret;
#if defined(LEVK_USE_GLFW)
	VkSurfaceKHR surface;
	auto result = glfwCreateWindowSurface(instance, nativeWindow.m_pWindow, nullptr, &surface);
	ASSERT(result == VK_SUCCESS, "Surface creation failed!");
	if (result == VK_SUCCESS)
	{
		ret = surface;
	}
#endif
	return ret;
}

void* WindowImpl::nativeHandle(WindowID window)
{
	if (auto pImpl = windowImpl(window); pImpl)
	{
#if defined(LEVK_USE_GLFW)
		return (void*)pImpl->m_pWindow->m_uImpl->m_uNativeWindow->m_pWindow;
#else
		LOG_E("NOT IMPLEMENTED");
		return nullptr;
#endif
	}
	return nullptr;
}

WindowID WindowImpl::guiWindow()
{
	if (gfx::ext_gui::isInit())
	{
		for (auto pWindow : g_registeredWindows)
		{
			auto const id = pWindow->m_pWindow->m_id;
			if (auto pImpl = rendererImpl(id); pImpl && pImpl->m_bGUI)
			{
				return id;
			}
		}
	}
	return {};
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

bool WindowImpl::create(Window::Info const& info)
{
	try
	{
		gfx::g_device.waitIdle();
		m_uNativeWindow = std::make_unique<NativeWindow>(info);
		gfx::RendererImpl::Info rendererInfo;
		rendererInfo.presenterInfo.config.getNewSurface = [this](vk::Instance instance) -> vk::SurfaceKHR {
			return createSurface(instance, *m_uNativeWindow);
		};
		rendererInfo.presenterInfo.config.getFramebufferSize = [this]() -> glm::ivec2 { return framebufferSize(); };
		rendererInfo.presenterInfo.config.getWindowSize = [this]() -> glm::ivec2 { return windowSize(); };
		rendererInfo.presenterInfo.config.window = m_pWindow->m_id;
		for (auto colourSpace : info.options.colourSpaces)
		{
			rendererInfo.presenterInfo.options.formats.push_back(gfx::g_colourSpaceMap.at((size_t)colourSpace));
		}
		if (os::isDefined("immediate"))
		{
			LOG_I("[{}] Immediate mode requested...", Window::s_tName);
			rendererInfo.presenterInfo.options.presentModes.push_back(gfx::g_presentModeMap.at((size_t)PresentMode::eImmediate));
		}
		for (auto presentMode : info.options.presentModes)
		{
			rendererInfo.presenterInfo.options.presentModes.push_back(gfx::g_presentModeMap.at((size_t)presentMode));
		}
		rendererInfo.frameCount = info.config.virtualFrameCount;
		rendererInfo.windowID = m_pWindow->id();
		rendererInfo.bGUI = info.config.bEnableGUI;
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
		if (info.options.bCentreCursor)
		{
			auto const size = m_uNativeWindow->windowSize();
			glfwSetCursorPos(m_uNativeWindow->m_pWindow, size.x / 2, size.y / 2);
		}
		glfwShowWindow(m_uNativeWindow->m_pWindow);
#endif
		m_pWindow->m_renderer.m_uImpl = std::make_unique<gfx::RendererImpl>(rendererInfo, &m_pWindow->m_renderer);
		LOG_D("[{}:{}] created", Window::s_tName, m_pWindow->m_id);
		return true;
	}
	catch (std::exception const& e)
	{
		LOG_E("[{}:{}] Failed to create window!\n\t{}", Window::s_tName, m_pWindow->m_id, e.what());
		m_pWindow->m_renderer.m_uImpl.reset();
		m_uNativeWindow.reset();
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

bool WindowImpl::isFocused() const
{
	bool bRet = false;
	if (m_uNativeWindow)
	{
#if defined(LEVK_USE_GLFW)
		bRet = glfwGetWindowAttrib(m_uNativeWindow->m_pWindow, GLFW_FOCUSED) != 0;
#endif
	}
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
			m_pWindow->m_renderer.m_uImpl.reset();
			m_uNativeWindow.reset();
			LOG_D("[{}:{}] closed", Window::s_tName, m_pWindow->m_id);
		}
		m_windowSize = m_framebufferSize = {};
#if defined(LEVK_USE_GLFW)
	}
#endif
	return;
}

void WindowImpl::onFramebufferSize(glm::ivec2 const& /*size*/)
{
	if (m_pWindow->m_renderer.m_uImpl)
	{
		m_pWindow->m_renderer.m_uImpl->onFramebufferResize();
	}
	return;
}

glm::ivec2 WindowImpl::framebufferSize() const
{
	return m_uNativeWindow ? m_uNativeWindow->framebufferSize() : glm::ivec2(0);
}

glm::ivec2 WindowImpl::windowSize() const
{
	return m_uNativeWindow ? m_uNativeWindow->windowSize() : glm::ivec2(0);
}

void WindowImpl::setCursorMode([[maybe_unused]] CursorMode mode) const
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
		return {(f32)x, (f32)y};
	}
#endif
	return {};
}

void WindowImpl::setCursorPos([[maybe_unused]] glm::vec2 const& pos)
{
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
	{
		glfwSetCursorPos(m_uNativeWindow->m_pWindow, pos.x, pos.y);
	}
#endif
	return;
}

std::string WindowImpl::clipboard() const
{
#if defined(LEVK_USE_GLFW)
	if (threads::isMainThread() && g_bGLFWInit && m_uNativeWindow && m_uNativeWindow->m_pWindow)
	{
		return glfwGetClipboardString(m_uNativeWindow->m_pWindow);
	}
#endif
	return {};
}

JoyState WindowImpl::joyState([[maybe_unused]] s32 id)
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

GamepadState WindowImpl::gamepadState([[maybe_unused]] s32 id)
{
	GamepadState ret;
#if defined(LEVK_USE_GLFW)
	GLFWgamepadstate glfwState;
	if (threads::isMainThread() && g_bGLFWInit && glfwJoystickIsGamepad(id) && glfwGetGamepadState(id, &glfwState))
	{
		ret.name = glfwGetGamepadName(id);
		ret.id = id;
		ret.joyState = joyState(id);
	}
#endif
	return ret;
}

f32 WindowImpl::triggerToAxis([[maybe_unused]] f32 triggerValue)
{
	f32 ret = triggerValue;
#if defined(LEVK_USE_GLFW)
	ret = (triggerValue + 1.0f) * 0.5f;
#endif
	return ret;
}

size_t WindowImpl::joystickAxesCount([[maybe_unused]] s32 id)
{
	size_t ret = 0;
#if defined(LEVK_USE_GLFW)
	s32 max;
	glfwGetJoystickAxes(id, &max);
	ret = size_t(max);
#endif
	return ret;
}

size_t WindowImpl::joysticKButtonsCount([[maybe_unused]] s32 id)
{
	size_t ret = 0;
#if defined(LEVK_USE_GLFW)
	s32 max;
	glfwGetJoystickButtons(id, &max);
	ret = size_t(max);
#endif
	return ret;
}

std::string_view WindowImpl::toString([[maybe_unused]] s32 key)
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

void WindowImpl::renderAll()
{
	for (auto pWindow : g_registeredWindows)
	{
		pWindow->m_pWindow->m_renderer.render();
	}
	return;
}
} // namespace le
