#pragma once
#include <vector>
#include <core/erased_ref.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>
#include <window/types.hpp>

namespace le::window {
template <typename T>
class TSurface {
  public:
	glm::ivec2 windowSize() const noexcept {
		return cast().windowSize();
	}
	glm::ivec2 framebufferSize() const noexcept {
		return cast().framebufferSize();
	}
	CursorType cursorType() const noexcept {
		return cast().cursorType();
	}
	CursorMode cursorMode() const noexcept {
		return cast().cursorMode();
	}
	glm::vec2 cursorPosition() const noexcept {
		return cast().cursorPosition();
	}

	void cursorType(CursorType type) {
		return cast().cursorType(type);
	}
	void cursorMode(CursorMode mode) {
		return cast().cursorMode(mode);
	}
	void cursorPosition(glm::vec2 position) {
		return cast().cursorPosition(position);
	}

	void show() {
		cast().show();
	}
	ErasedRef nativePtr() const noexcept {
		return cast().nativePtr();
	}

	Span<std::string_view> vkInstanceExtensions() const {
		return cast().vkInstanceExtensions();
	}
	bool vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const {
		return cast().vkCreateSurface(vkInstance, out_vkSurface);
	}

  private:
	T& cast() {
		return static_cast<T&>(this);
	}

	T const& cast() const {
		return static_cast<T const&>(this);
	}
};
} // namespace le::window
