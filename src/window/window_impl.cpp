#include <array>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <editor/editor.hpp>
#include <gfx/common.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/renderer_impl.hpp>
#if defined(LEVK_USE_GLFW)
#if defined(LEVK_RUNTIME_MSVC)
#include <Windows.h>
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
#include <window/window_impl.hpp>

namespace le
{
using namespace input;

namespace
{
std::unordered_set<WindowImpl*> g_registeredWindows;
#if defined(LEVK_EDITOR)
WindowImpl* g_pEditorWindow = nullptr;
#endif

#if defined(LEVK_USE_GLFW)
bool g_bGLFWInit = false;

void onGLFWError(s32 code, char const* desc)
{
	LOG_E("[{}] GLFW Error! [{}]: {}", Window::s_tName, code, desc);
	return;
}

void onWindowResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
{
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		pWindow->m_windowSize = {width, height};
		WindowImpl::s_input[pWindow->m_pWindow->id()].onWindowResize(width, height);
		LOG_D("[{}:{}] Window resized: [{}x{}]", Window::s_tName, pWindow->m_pWindow->id(), width, height);
	}
	return;
}

void onFramebufferResize(GLFWwindow* pGLFWwindow, s32 width, s32 height)
{
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		pWindow->onFramebufferSize({width, height});
		LOG_D("[{}:{}] Framebuffer resized: [{}x{}]", Window::s_tName, pWindow->m_pWindow->id(), width, height);
	}
	return;
}

void onKey(GLFWwindow* pGLFWwindow, s32 key, s32 /*scancode*/, s32 action, s32 mods)
{
	WindowImpl::s_input[WindowID::s_null].onInput(Key(key), Action(action), Mods::VALUE(mods));
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onInput(Key(key), Action(action), Mods::VALUE(mods));
	}
	return;
}

void onMouse(GLFWwindow* pGLFWwindow, f64 x, f64 y)
{
	WindowImpl::s_input[WindowID::s_null].onMouse(x, y);
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onMouse(x, y);
	}
	return;
}

void onMouseButton(GLFWwindow* pGLFWwindow, s32 key, s32 action, s32 mods)
{
	WindowImpl::s_input[WindowID::s_null].onInput(Key(key + (s32)Key::eMouseButton1), Action(action), Mods::VALUE(mods));
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onInput(Key(key + (s32)Key::eMouseButton1), Action(action), Mods::VALUE(mods));
	}
	return;
}

void onText(GLFWwindow* pGLFWwindow, u32 codepoint)
{
	WindowImpl::s_input[WindowID::s_null].onText(static_cast<char>(codepoint));
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onText(static_cast<char>(codepoint));
	}
	return;
}

void onScroll(GLFWwindow* pGLFWwindow, f64 dx, f64 dy)
{
	WindowImpl::s_input[WindowID::s_null].onScroll(dx, dy);
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onScroll(dx, dy);
	}
	return;
}

void onFiledrop(GLFWwindow* pGLFWwindow, s32 count, char const** szPaths)
{
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		for (std::size_t idx = 0; idx < (std::size_t)count; ++idx)
		{
			stdfs::path path(szPaths[idx]);
		}
	}
	return;
}

void onFocus(GLFWwindow* pGLFWwindow, s32 entered)
{
	if (auto pWindow = WindowImpl::find(pGLFWwindow); pWindow)
	{
		WindowImpl::s_input[pWindow->m_pWindow->id()].onFocus(entered != 0);
	}
	return;
}

WindowImpl::Cursor const& getCursor(input::CursorType type)
{
	auto& cursor = WindowImpl::s_cursors.at((std::size_t)type);
	if (type != input::CursorType::eDefault && !cursor.data.contains<GLFWcursor*>())
	{
		s32 gCursor = 0;
		switch (type)
		{
		default:
		case input::CursorType::eDefault:
			break;
		case input::CursorType::eResizeEW:
			gCursor = GLFW_RESIZE_EW_CURSOR;
			break;
		case input::CursorType::eResizeNS:
			gCursor = GLFW_RESIZE_NS_CURSOR;
			break;
		case input::CursorType::eResizeNWSE:
			gCursor = GLFW_RESIZE_NWSE_CURSOR;
			break;
		case input::CursorType::eResizeNESW:
			gCursor = GLFW_RESIZE_NESW_CURSOR;
			break;
		}
		if (gCursor != 0)
		{
			cursor.data = glfwCreateStandardCursor(gCursor);
		}
	}
	cursor.type = type;
	return cursor;
}
#endif

void registerCallbacks(NativeWindow const& window)
{
#if defined(LEVK_USE_GLFW)
	if (auto pWindow = window.cast<GLFWwindow>())
	{
		glfwSetWindowSizeCallback(pWindow, &onWindowResize);
		glfwSetFramebufferSizeCallback(pWindow, &onFramebufferResize);
		glfwSetKeyCallback(pWindow, &onKey);
		glfwSetCharCallback(pWindow, &onText);
		glfwSetCursorPosCallback(pWindow, &onMouse);
		glfwSetMouseButtonCallback(pWindow, &onMouseButton);
		glfwSetScrollCallback(pWindow, &onScroll);
		glfwSetDropCallback(pWindow, &onFiledrop);
		glfwSetCursorEnterCallback(pWindow, &onFocus);
	}
#endif
}

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

f32 Gamepad::axis(Axis axis) const
{
	[[maybe_unused]] std::size_t idx = std::size_t(axis);
#if defined(LEVK_USE_GLFW)
	s32 max = 0;
	glfwGetJoystickAxes(id, &max);
	if (idx < (std::size_t)max && idx < joyState.axes.size())
	{
		return joyState.axes.at(idx);
	}
#endif
	return 0.0f;
}

bool Gamepad::isPressed(Key button) const
{
	[[maybe_unused]] std::size_t idx = (std::size_t)button - (std::size_t)Key::eGamepadButtonA;
#if defined(LEVK_USE_GLFW)
	s32 max = 0;
	glfwGetJoystickButtons(id, &max);
	if (idx < (std::size_t)max && idx < joyState.buttons.size())
	{
		return joyState.buttons.at(idx);
	}
#endif
	return false;
}

std::unordered_map<WindowID, WindowImpl::InputCallbacks> WindowImpl::s_input;
EnumArray<WindowImpl::Cursor, input::CursorType> WindowImpl::s_cursors;

WindowImpl* WindowImpl::find(StaticAny<> nativeHandle)
{
#if defined(LEVK_USE_GLFW)
	auto f = [nativeHandle](auto pWindow) -> bool {
		return pWindow->m_uNativeWindow && pWindow->m_uNativeWindow->template cast<GLFWwindow>() == nativeHandle.get<GLFWwindow*>();
	};
	auto search = std::find_if(g_registeredWindows.begin(), g_registeredWindows.end(), f);
	return search != g_registeredWindows.end() ? *search : nullptr;
#else
	return nullptr;
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
	g_bGLFWInit = true;
#endif
	return true;
}

void WindowImpl::deinit()
{
#if defined(LEVK_USE_GLFW)
	for (auto& cursor : s_cursors)
	{
		auto pCursor = cursor.data.get<GLFWcursor*>();
		if (pCursor)
		{
			glfwDestroyCursor(pCursor);
		}
	}
	glfwTerminate();
	LOG_D("[{}] GLFW terminated", Window::s_tName);
	g_bGLFWInit = false;
#endif
	s_input.clear();
	s_cursors = {};
	return;
}

void WindowImpl::update()
{
#if defined(LEVK_EDITOR)
	if (g_pEditorWindow && g_pEditorWindow->isOpen())
	{
		gfx::ext_gui::newFrame();
	}
#endif
	for (auto pWindow : g_registeredWindows)
	{
		if (auto pRenderer = pWindow->m_pWindow->m_renderer.m_uImpl.get())
		{
			pRenderer->update();
		}
	}
}

std::vector<char const*> WindowImpl::vulkanInstanceExtensions()
{
	std::vector<char const*> ret;
#if defined(LEVK_USE_GLFW)
	u32 glfwExtCount;
	char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
	ret.reserve((std::size_t)glfwExtCount);
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

std::unordered_set<s32> WindowImpl::allExisting()
{
	std::unordered_set<s32> ret;
	for (auto pImpl : g_registeredWindows)
	{
		if (pImpl->exists())
		{
			ret.insert(pImpl->m_pWindow->m_id);
		}
	}
	return ret;
}

StaticAny<> WindowImpl::nativeHandle(WindowID window)
{
	if (auto pImpl = windowImpl(window); pImpl)
	{
		return pImpl->m_pWindow->m_uImpl->m_uNativeWindow->m_window;
	}
	return nullptr;
}

WindowID WindowImpl::editorWindow()
{
#if defined(LEVK_EDITOR)
	if (gfx::ext_gui::isInit())
	{
		for (auto pWindow : g_registeredWindows)
		{
			if (pWindow == g_pEditorWindow)
			{
				return pWindow->m_pWindow->m_id;
			}
		}
	}
#endif
	return {};
}

WindowImpl::WindowImpl(Window* pWindow) : m_pWindow(pWindow)
{
	registerWindow(this);
}

WindowImpl::~WindowImpl()
{
#if defined(LEVK_EDITOR)
	if (this == g_pEditorWindow)
	{
		gfx::deferred::release([]() {
			editor::deinit();
			gfx::ext_gui::deinit();
		});
		g_pEditorWindow = nullptr;
	}
#endif
	unregisterWindow(this);
	close();
	destroy();
}

bool WindowImpl::create(Window::Info const& info)
{
	try
	{
		m_uNativeWindow = std::make_unique<NativeWindow>(info);
		gfx::RendererImpl::Info rendererInfo;
		rendererInfo.contextInfo.config.getNewSurface = [this](vk::Instance instance) -> vk::SurfaceKHR { return m_uNativeWindow->createSurface(instance); };
		rendererInfo.contextInfo.config.getFramebufferSize = [this]() -> glm::ivec2 { return framebufferSize(); };
		rendererInfo.contextInfo.config.getWindowSize = [this]() -> glm::ivec2 { return windowSize(); };
		rendererInfo.contextInfo.config.window = m_pWindow->m_id;
		for (auto colourSpace : info.options.colourSpaces)
		{
			rendererInfo.contextInfo.options.formats.push_back(gfx::g_colourSpaceMap.at((std::size_t)colourSpace));
		}
		if (os::isDefined("immediate"))
		{
			LOG_I("[{}] Immediate mode requested...", Window::s_tName);
			rendererInfo.contextInfo.options.presentModes.push_back((vk::PresentModeKHR)PresentMode::eImmediate);
		}
		for (auto presentMode : info.options.presentModes)
		{
			rendererInfo.contextInfo.options.presentModes.push_back((vk::PresentModeKHR)presentMode);
		}
		rendererInfo.frameCount = info.config.virtualFrameCount;
		rendererInfo.windowID = m_pWindow->id();
		registerCallbacks(*m_uNativeWindow);
		m_uNativeWindow->show(info.options.bCentreCursor);
		m_pWindow->m_renderer.m_uImpl = std::make_unique<gfx::RendererImpl>(rendererInfo, &m_pWindow->m_renderer);
		m_presentModes.clear();
		m_presentModes.reserve(m_pWindow->m_renderer.m_uImpl->m_context.m_metadata.presentModes.size());
		for (auto mode : m_pWindow->m_renderer.m_uImpl->m_context.m_metadata.presentModes)
		{
			m_presentModes.push_back((PresentMode)mode);
		}
#if defined(LEVK_EDITOR)
		if (!g_pEditorWindow && !gfx::ext_gui::isInit())
		{
			if (!m_pWindow->m_renderer.m_uImpl->initExtGUI())
			{
				LOG_E("[{}] Failed to initialise Editor!", Window::s_tName);
			}
			else
			{
				g_pEditorWindow = this;
				editor::init(m_pWindow->m_id);
			}
		}
#endif
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
	if (g_bGLFWInit && m_uNativeWindow)
	{
		bRet = !glfwWindowShouldClose(m_uNativeWindow->cast<GLFWwindow>());
	}
#endif
	return bRet;
}

bool WindowImpl::exists() const
{
	bool bRet = false;
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		bRet = m_uNativeWindow->cast<GLFWwindow>() != nullptr;
	}
#endif
	return bRet;
}

bool WindowImpl::isClosing() const
{
	bool bRet = false;
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		bRet = glfwWindowShouldClose(m_uNativeWindow->cast<GLFWwindow>());
	}
#endif
	return bRet;
}

bool WindowImpl::isFocused() const
{
	bool bRet = false;
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		bRet = glfwGetWindowAttrib(m_uNativeWindow->cast<GLFWwindow>(), GLFW_FOCUSED) != 0;
	}
#endif
	return bRet;
}

void WindowImpl::close()
{
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		glfwSetWindowShouldClose(m_uNativeWindow->cast<GLFWwindow>(), 1);
	}
#endif
	return;
}

void WindowImpl::destroy()
{
#if defined(LEVK_USE_GLFW)
	if (!g_bGLFWInit)
	{
		return;
	}
#endif
	if (m_uNativeWindow)
	{
		m_pWindow->m_renderer.m_uImpl.reset();
		m_uNativeWindow.reset();
		LOG_D("[{}:{}] closed", Window::s_tName, m_pWindow->m_id);
	}
	m_windowSize = m_framebufferSize = {};
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

PresentMode WindowImpl::presentMode() const
{
	return m_pWindow->m_renderer.m_uImpl ? (PresentMode)m_pWindow->m_renderer.m_uImpl->presentMode() : PresentMode::eFifo;
}

bool WindowImpl::setPresentMode(PresentMode mode)
{
	return m_pWindow->m_renderer.m_uImpl ? m_pWindow->m_renderer.m_uImpl->setPresentMode((vk::PresentModeKHR)mode) : false;
}

glm::ivec2 WindowImpl::framebufferSize() const
{
	return m_uNativeWindow ? m_uNativeWindow->framebufferSize() : glm::ivec2(0);
}

glm::ivec2 WindowImpl::windowSize() const
{
	return m_uNativeWindow ? m_uNativeWindow->windowSize() : glm::ivec2(0);
}

void WindowImpl::setCursorType([[maybe_unused]] CursorType type)
{
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		if (type != m_cursor.type)
		{
			m_cursor = getCursor(type);
			glfwSetCursor(m_uNativeWindow->cast<GLFWwindow>(), m_cursor.data.get<GLFWcursor*>());
		}
	}
#endif
}

void WindowImpl::setCursorMode([[maybe_unused]] CursorMode mode) const
{
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
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
			val = glfwGetInputMode(m_uNativeWindow->cast<GLFWwindow>(), GLFW_CURSOR);
			break;
		}
		glfwSetInputMode(m_uNativeWindow->cast<GLFWwindow>(), GLFW_CURSOR, val);
	}
#endif
	return;
}

CursorMode WindowImpl::cursorMode() const
{
	CursorMode ret = CursorMode::eDefault;
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		s32 val = glfwGetInputMode(m_uNativeWindow->cast<GLFWwindow>(), GLFW_CURSOR);
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
	glm::vec2 ret = {};
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		f64 x, y;
		glfwGetCursorPos(m_uNativeWindow->cast<GLFWwindow>(), &x, &y);
		ret = {(f32)x, (f32)y};
	}
#endif
	return ret;
}

void WindowImpl::setCursorPos([[maybe_unused]] glm::vec2 const& pos)
{
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		glfwSetCursorPos(m_uNativeWindow->cast<GLFWwindow>(), pos.x, pos.y);
	}
#endif
	return;
}

std::string WindowImpl::clipboard() const
{
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit && m_uNativeWindow)
	{
		return glfwGetClipboardString(m_uNativeWindow->cast<GLFWwindow>());
	}
#endif
	return {};
}

bool WindowImpl::anyActive()
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->isOpen())
		{
			return true;
		}
	}
	return false;
}

bool WindowImpl::anyExist()
{
	for (auto pWindow : g_registeredWindows)
	{
		if (pWindow->exists())
		{
			return true;
		}
	}
	return false;
}

void WindowImpl::pollEvents()
{
#if defined(LEVK_USE_GLFW)
	if (g_bGLFWInit)
	{
		glfwPollEvents();
	}
#endif
	return;
}

void WindowImpl::renderAll()
{
#if defined(LEVK_GLFW)
	if (!g_bGLFWInit)
	{
		return;
	}
#endif
	bool bExtGUI = false;
	for (auto pWindow : g_registeredWindows)
	{
		if (!pWindow->isClosing())
		{
#if defined(LEVK_EDITOR)
			if (g_pEditorWindow)
			{
				bExtGUI = g_pEditorWindow == pWindow;
			}
#endif
			pWindow->m_pWindow->m_renderer.render(bExtGUI);
		}
	}
	return;
}
} // namespace le
