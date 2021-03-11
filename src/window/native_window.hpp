#pragma once
#include <engine/window/window.hpp>
#include <glm/vec2.hpp>
#include <kt/fixed_any/fixed_any.hpp>
#include <vulkan/vulkan.hpp>

namespace le {
class NativeWindow final {
  public:
	kt::fixed_any_t<> m_window;

  public:
	NativeWindow() noexcept = default;
	NativeWindow(Window::Info const& info);
	NativeWindow(NativeWindow&&) = default;
	NativeWindow& operator=(NativeWindow&&);
	~NativeWindow();

  public:
	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

	vk::SurfaceKHR createSurface(vk::Instance instance) const;
	void show(bool bCentreCursor) const;

	template <typename T>
	T* cast() const {
		return m_window.template value_or<T*>(nullptr);
	}
};
} // namespace le
