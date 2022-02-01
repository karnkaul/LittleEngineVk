#pragma once
#if defined(LEVK_USE_GLFW)
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <levk/core/span.hpp>
#include <levk/window/window.hpp>
#include <vulkan/vulkan.hpp>

namespace le::window {
Span<std::string_view const> instanceExtensions(Window const& win);
vk::SurfaceKHR makeSurface(vk::Instance vkInst, Window const& win);
GLFWwindow* glfwPtr(Window const& win);
} // namespace le::window
#endif
