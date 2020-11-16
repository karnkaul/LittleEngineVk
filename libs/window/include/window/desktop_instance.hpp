#pragma once
#if defined(LEVK_USE_GLFW)
#include <window/instance.hpp>

namespace le::window {
class DesktopInstance final : public Instance<DesktopInstance> {
  public:
	DesktopInstance() : Instance(false) {
	}
	explicit DesktopInstance(CreateInfo const& info);
	DesktopInstance(DesktopInstance&&) = default;
	DesktopInstance& operator=(DesktopInstance&&) = default;
	~DesktopInstance();

	// ISurface
	glm::ivec2 windowSize() const noexcept;
	glm::ivec2 framebufferSize() const noexcept;
	CursorType cursorType() const noexcept;
	CursorMode cursorMode() const noexcept;
	glm::vec2 cursorPosition() const noexcept;

	void cursorType(CursorType type);
	void cursorMode(CursorMode mode);
	void cursorPosition(glm::vec2 position);

	void show();
	ErasedRef nativePtr() const noexcept;

	Span<std::string_view> vkInstanceExtensions() const;
	bool vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const;

	// Instance
	EventQueue pollEvents();
	void close();

	// Desktop
	bool importControllerDB(std::string_view db) const;
	std::vector<Gamepad> activeGamepads() const;
	Joystick joyState(s32 id) const;
	Gamepad gamepadState(s32 id) const;
	f32 triggerToAxis(f32 triggerValue) const;
	std::size_t joystickAxesCount(s32 id) const;
	std::size_t joysticKButtonsCount(s32 id) const;
	std::string_view toString(s32 key) const;
};
} // namespace le::window
#endif
