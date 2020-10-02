#include <core/assert.hpp>
#include <core/log.hpp>
#include <window/native_window.hpp>
#if defined(LEVK_USE_GLFW)
#include <GLFW/glfw3.h>
#endif

namespace le {
NativeWindow::NativeWindow([[maybe_unused]] Window::Info const& info) {
#if defined(LEVK_USE_GLFW)
	s32 screenCount;
	GLFWmonitor** ppScreens = glfwGetMonitors(&screenCount);
	if (screenCount < 1) {
		LOG_E("[{}] Failed to detect screens!", Window::s_tName);
		throw std::runtime_error("Failed to create Window");
	}
	GLFWvidmode const* mode = glfwGetVideoMode(ppScreens[0]);
	if (!mode) {
		LOG_E("[{}] Failed to detect video mode!", Window::s_tName);
		throw std::runtime_error("Failed to create Window");
	}
	std::size_t screenIdx = info.options.screenID < screenCount ? (std::size_t)info.options.screenID : 0;
	GLFWmonitor* pTarget = ppScreens[screenIdx];
	s32 height = info.config.size.y;
	s32 width = info.config.size.x;
	bool bDecorated = true;
	switch (info.config.mode) {
	default:
	case Window::Mode::eDecoratedWindow: {
		if (mode->width < width || mode->height < height) {
			LOG_E("[{}] Window size [{}x{}] too large for default screen! [{}x{}]", Window::s_tName, width, height, mode->width, mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		pTarget = nullptr;
		break;
	}
	case Window::Mode::eBorderlessWindow: {
		if (mode->width < width || mode->height < height) {
			LOG_E("[{}] Window size [{}x{}] too large for default screen! [{}x{}]", Window::s_tName, width, height, mode->width, mode->height);
			throw std::runtime_error("Failed to create Window");
		}
		bDecorated = false;
		pTarget = nullptr;
		break;
	}
	case Window::Mode::eBorderlessFullscreen: {
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
	ASSERT(cX >= 0 && cY >= 0 && cX < mode->width && cY < mode->height, "Invalid centre-screen!");
	glfwWindowHint(GLFW_DECORATED, bDecorated ? 1 : 0);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_VISIBLE, false);
	auto pWindow = glfwCreateWindow(width, height, info.config.title.data(), pTarget, nullptr);
	if (!pWindow) {
		throw std::runtime_error("Failed to create Window");
	}
	m_window = pWindow;
	glfwSetWindowPos(pWindow, cX, cY);
#else
	throw std::runtime_error("Unsupported configuration");
#endif
}

NativeWindow& NativeWindow::operator=(NativeWindow&& rhs) {
	if (&rhs != this) {
#if defined(LEVK_USE_GLFW)
		glfwDestroyWindow(cast<GLFWwindow>());
#endif
		m_window = rhs.m_window;
		rhs.m_window.clear();
	}
	return *this;
}

NativeWindow::~NativeWindow() {
#if defined(LEVK_USE_GLFW)
	glfwDestroyWindow(cast<GLFWwindow>());
#endif
}

glm::ivec2 NativeWindow::windowSize() const {
	glm::ivec2 ret = {};
#if defined(LEVK_USE_GLFW)
	glfwGetWindowSize(cast<GLFWwindow>(), &ret.x, &ret.y);
#endif
	return ret;
}

glm::ivec2 NativeWindow::framebufferSize() const {
	glm::ivec2 ret = {};
#if defined(LEVK_USE_GLFW)
	glfwGetFramebufferSize(cast<GLFWwindow>(), &ret.x, &ret.y);
#endif
	return ret;
}

vk::SurfaceKHR NativeWindow::createSurface([[maybe_unused]] vk::Instance instance) const {
	vk::SurfaceKHR ret;
#if defined(LEVK_USE_GLFW)
	VkSurfaceKHR surface;
	auto result = glfwCreateWindowSurface(instance, cast<GLFWwindow>(), nullptr, &surface);
	ASSERT(result == VK_SUCCESS, "Surface creation failed!");
	if (result == VK_SUCCESS) {
		ret = surface;
	}
#endif
	return ret;
}

void NativeWindow::show([[maybe_unused]] bool bCentreCursor) const {
#if defined(LEVK_USE_GLFW)
	auto pWindow = cast<GLFWwindow>();
	if (bCentreCursor) {
		auto const size = windowSize();
		glfwSetCursorPos(pWindow, size.x / 2, size.y / 2);
	}
	glfwShowWindow(pWindow);
#endif
}
} // namespace le
