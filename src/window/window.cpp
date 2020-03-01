#include <mutex>
#include <unordered_set>
#if defined(LEVK_USE_GLFW)
// TODO: Enable after Vulkan CMake integration
// #define GLFW_INCLUDE_VULKAN
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
} // namespace

class WindowImpl final
{
public:
#if defined(LEVK_USE_GLFW)
	GLFWwindow* m_pWindow = nullptr;
#endif

	~WindowImpl()
	{
		close();
		destroy();
	}

	bool create(Window::Data const& data)
	{
#if defined(LEVK_USE_GLFW)
		// s32 count;
		// auto ppMonitors = glfwGetMonitors(&count);
		GLFWmonitor* pTarget = nullptr;
		if (data.mode != Window::Mode::DecoratedWindow)
		{
			LOG_E("NOT IMPLEMENTED");
			return false;
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_pWindow = glfwCreateWindow(data.size.x, data.size.y, data.title.data(), pTarget, nullptr);
		if (m_pWindow)
		{
			return true;
		}
#endif
		LOG_E("[{}] Failed to create window!", utils::tName<Window>());
		return false;
	}

	bool isOpen()
	{
#if defined(LEVK_USE_GLFW)
		return m_pWindow != nullptr && !glfwWindowShouldClose(m_pWindow);
#endif
	}

	bool exists()
	{
#if defined(LEVK_USE_GLFW)
		return m_pWindow != nullptr;
#else
		return false;
#endif
	}

	bool isClosing()
	{
#if defined(LEVK_USE_GLFW)
		return m_pWindow != nullptr && glfwWindowShouldClose(m_pWindow);
#else
		return false;
#endif
	}

	void pollEvents()
	{
#if defined(LEVK_USE_GLFW)
		if (m_pWindow)
		{
			glfwPollEvents();
		}
#endif
		return;
	}

	void close()
	{
#if defined(LEVK_USE_GLFW)
		if (m_pWindow)
		{
			glfwSetWindowShouldClose(m_pWindow, 1);
		}
#endif
		return;
	}

	void destroy()
	{
#if defined(LEVK_USE_GLFW)
		if (m_pWindow)
		{
			glfwDestroyWindow(m_pWindow);
			m_pWindow = nullptr;
		}
#endif
		return;
	}
};

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

Window::ID Window::id() const
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

bool Window::create(Data const& data)
{
	return m_uImpl ? m_uImpl->create(data) : false;
}

void Window::pollEvents()
{
	if (m_uImpl)
	{
		m_uImpl->pollEvents();
	}
	return;
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
} // namespace le
