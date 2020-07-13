#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>
#include <engine/window/window.hpp>

namespace le
{
class NativeWindow final
{
public:
	void* m_pWindow = nullptr;

public:
	NativeWindow(Window::Info const& info);
	~NativeWindow();

public:
	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

	vk::SurfaceKHR createSurface(vk::Instance instance) const;
	void show(bool bCentreCursor) const;

	template <typename T>
	T* cast() const
	{
		return reinterpret_cast<T*>(m_pWindow);
	}
};
} // namespace le
