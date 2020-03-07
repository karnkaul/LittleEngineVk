#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "engine/window/window.hpp"

namespace le
{
#if defined(LEVK_USE_GLFW)
inline bool g_bGLFWInit = false;
#endif

struct NativeSurface final
{
	vk::SurfaceKHR surface;

	NativeSurface(vk::Instance const& instance);
	~NativeSurface();

	explicit operator vk::SurfaceKHR const&() const;
};

class NativeWindow final
{
public:
#if defined(LEVK_USE_GLFW)
	class GLFWwindow* m_pWindow = nullptr;
#endif
	glm::ivec2 m_size = {};
	glm::ivec2 m_initialCentre = {};

public:
	NativeWindow(Window::Data const& data);
	~NativeWindow();
};
} // namespace le
