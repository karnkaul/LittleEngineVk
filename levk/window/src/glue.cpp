#include <impl.hpp>
#include <levk/window/glue.hpp>

#if defined(LEVK_USE_GLFW)
namespace le::window {
Span<std::string_view const> instanceExtensions(Window const&) {
	static std::vector<std::string_view> ret;
	if (ret.empty()) {
		u32 glfwExtCount;
		char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		ret.reserve((std::size_t)glfwExtCount);
		for (u32 i = 0; i < glfwExtCount; ++i) { ret.push_back(glfwExtensions[i]); }
	}
	return ret;
}

vk::SurfaceKHR makeSurface(vk::Instance vkInst, Window const& win) {
	VkSurfaceKHR surface;
	auto result = glfwCreateWindowSurface(static_cast<VkInstance>(vkInst), glfwPtr(win), nullptr, &surface);
	if (result == VK_SUCCESS) { return vk::SurfaceKHR(surface); }
	return {};
}

Window::Impl& impl(Window const& win) { return *win.m_impl; }

GLFWwindow* glfwPtr(Window const& win) { return *impl(win).m_win; }
} // namespace le::window
#endif
