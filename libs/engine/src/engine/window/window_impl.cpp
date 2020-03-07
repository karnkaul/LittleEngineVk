#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/threads.hpp"
#include "core/utils.hpp"
#include "engine/vuk/instance/instance_impl.hpp"
#include "engine/vuk/context/swapchain.hpp"
#include "window_impl.hpp"

namespace le
{
namespace
{
std::unique_ptr<NativeWindow> g_uDummyWindow;
} // namespace

NativeSurface::NativeSurface(vk::Instance const& instance)
{
	Window::Data data;
	data.size = {128, 128};
	g_uDummyWindow = std::make_unique<NativeWindow>(data);
	VkSurfaceKHR surface;
#if defined(LEVK_USE_GLFW)
	if (g_uDummyWindow)
	{
		if (glfwCreateWindowSurface(static_cast<VkInstance>(instance), g_uDummyWindow->m_pWindow, nullptr, &surface) != VK_SUCCESS)
		{
			LOG_E("[{}] Failed to create [{}]", Window::s_tName, utils::tName<vk::SurfaceKHR>());
			g_uDummyWindow.reset();
			throw std::runtime_error("Failed to create Window");
		}
	}
#endif
	this->surface = surface;
}

NativeSurface::~NativeSurface()
{
	g_uDummyWindow.reset();
}

NativeSurface::operator vk::SurfaceKHR const&() const
{
	return surface;
}

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
	case Window::Mode::DecoratedWindow:
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
	case Window::Mode::BorderlessWindow:
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
	case Window::Mode::BorderlessFullscreen:
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
} // namespace le
