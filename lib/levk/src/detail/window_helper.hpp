#pragma once
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <levk/core/ptr.hpp>

namespace levk::detail {
[[nodiscard]] inline auto get_cursor_position(Ptr<GLFWwindow> window) -> glm::ivec2 {
	auto ret = glm::dvec2{};
	glfwGetCursorPos(window, &ret.x, &ret.y);
	return ret;
}

[[nodiscard]] inline auto get_window_size(Ptr<GLFWwindow> window) -> glm::ivec2 {
	auto ret = glm::ivec2{};
	glfwGetWindowSize(window, &ret.x, &ret.y);
	return ret;
}

[[nodiscard]] inline auto get_framebuffer_size(Ptr<GLFWwindow> window) -> glm::ivec2 {
	auto ret = glm::ivec2{};
	glfwGetFramebufferSize(window, &ret.x, &ret.y);
	return ret;
}

[[nodiscard]] inline auto get_instance_extensions() -> std::vector<char const*> {
	auto ret = std::vector<char const*>{};
	auto count = std::uint32_t{};
	auto const* array = glfwGetRequiredInstanceExtensions(&count);
	return {array, array + static_cast<std::ptrdiff_t>(count)}; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

[[nodiscard]] inline auto create_surface(Ptr<GLFWwindow> window, vk::Instance instance) -> vk::UniqueSurfaceKHR {
	VkSurfaceKHR ret{};
	glfwCreateWindowSurface(instance, window, nullptr, &ret);
	return vk::UniqueSurfaceKHR{ret, instance};
}
} // namespace levk::detail
