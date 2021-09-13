#pragma once
#if defined(LEVK_USE_GLFW)
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <core/span.hpp>
#include <vulkan/vulkan.hpp>
#include <window/instance.hpp>

namespace le::window {
Span<std::string_view const> instanceExtensions(Instance const& wInst);
vk::SurfaceKHR makeSurface(vk::Instance vkInst, Instance const& wInst);
GLFWwindow* glfwPtr(Instance const& wInst);
} // namespace le::window
#endif
