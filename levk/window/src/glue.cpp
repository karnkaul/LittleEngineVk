#include <instance_impl.hpp>
#include <levk/window/glue.hpp>

#if defined(LEVK_USE_GLFW)
namespace le::window {
Span<std::string_view const> instanceExtensions(Instance const&) {
	static std::vector<std::string_view> ret;
	if (ret.empty()) {
		u32 glfwExtCount;
		char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		ret.reserve((std::size_t)glfwExtCount);
		for (u32 i = 0; i < glfwExtCount; ++i) { ret.push_back(glfwExtensions[i]); }
	}
	return ret;
}

vk::SurfaceKHR makeSurface(vk::Instance vkInst, Instance const& wInst) {
	VkSurfaceKHR surface;
	auto result = glfwCreateWindowSurface(static_cast<VkInstance>(vkInst), glfwPtr(wInst), nullptr, &surface);
	if (result == VK_SUCCESS) { return vk::SurfaceKHR(surface); }
	return {};
}

Instance::Impl& impl(Instance const& wInst) { return *wInst.m_impl; }

GLFWwindow* glfwPtr(Instance const& wInst) { return *impl(wInst).m_win; }
} // namespace le::window
#endif
