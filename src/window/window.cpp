#include <mutex>
#include <unordered_set>
#if defined(LEVK_USE_GLFW)
#include <GLFW/glfw3.h>
#endif
#include "core/log.hpp"
#include "core/utils.hpp"
#include "window.hpp"

namespace le
{
namespace
{
using Lock = std::lock_guard<std::mutex>;
std::mutex g_windowMutex;
Window::ID g_nextWindowID = Window::ID::Null;
std::unordered_set<Window*> g_registeredWindows;

#if defined(LEVK_USE_GLFW)
void onGLFWError(s32 code, char const* desc)
{
	LOG_E("[{}] GLFW Error! [{}]: {}", utils::tName<Window>(), code, desc);
	return;
}
#endif

bool init()
{
#if defined(LEVK_USE_GLFW)
	glfwSetErrorCallback(&onGLFWError);
	if (!glfwInit())
	{
		LOG_E("[{}] Could not initialise GLFW!", utils::tName<Window>());
		return false;
	}
	else if (!glfwVulkanSupported())
	{
		LOG_E("[{}] Vulkan not supported!", utils::tName<Window>());
		return false;
	}
	else
	{
		LOG_D("[{}] GLFW initialised successfully", utils::tName<Window>());
	}
#endif
	return true;
}

void deinit()
{
#if defined(LEVK_USE_GLFW)
	glfwTerminate();
	LOG_D("[{}] GLFW terminated", utils::tName<Window>());
#endif
	return;
}

bool registerWindow(Window* pWindow)
{
	if (g_registeredWindows.empty())
	{
		if (!init())
		{
			return false;
		}
	}
	g_registeredWindows.insert(pWindow);
	LOG_D("New Window registered. Active: [{}]", g_registeredWindows.size());
	return true;
}

void unregisterWindow(Window* pWindow)
{
	if (auto search = g_registeredWindows.find(pWindow); search != g_registeredWindows.end())
	{
		g_registeredWindows.erase(search);
		LOG_D("Window deregistered. Active: [{}]", g_registeredWindows.size());
		if (g_registeredWindows.empty())
		{
			deinit();
		}
	}
}
}

class WindowImpl final
{
public:
#if defined(LEVK_USE_GLFW)
	static bool s_bGLFWInit;

	GLFWwindow* m_pWindow = nullptr;
#endif

	WindowImpl()
	{
		
	}

	~WindowImpl()
	{
		
	}
};

#if defined(LEVK_USE_GLFW)
bool WindowImpl::s_bGLFWInit = false;
#endif

Window::Window()
{
	Lock lock(g_windowMutex);
	if (registerWindow(this))
	{
		m_id = ++g_nextWindowID.handle;
		m_uImpl = std::make_unique<WindowImpl>();
		LOG_I("[{}] Window #{} constructed", utils::tName<Window>(), m_id);
	}
	else
	{
		throw std::runtime_error("Failed to construct Window object");
	}
}

Window::Window(Window&&) = default;
Window& Window::operator=(Window&&) = default;
Window::~Window()
{
	Lock lock(g_windowMutex);
	LOG_I("[{}] Window #{} destroyed", utils::tName<Window>(), m_id);
	unregisterWindow(this);
}


} // namespace le
