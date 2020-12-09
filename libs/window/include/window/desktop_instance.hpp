#pragma once
#if defined(LEVK_USE_GLFW)
#include <window/instance.hpp>

namespace le::window {
enum class Style { eDecoratedWindow = 0, eBorderlessWindow, eBorderlessFullscreen, eDedicatedFullscreen, eCOUNT_ };
constexpr static EnumArray<Style> const styleNames = {"Decorated Window", "Borderless Window", "Borderless Fullscreen", "Dedicated Fullscreen"};

class DesktopInstance final : public IInstance {
  public:
	struct CreateInfo;

	DesktopInstance() : IInstance(false) {
	}
	explicit DesktopInstance(CreateInfo const& info);
	DesktopInstance(DesktopInstance&&) = default;
	DesktopInstance& operator=(DesktopInstance&&) = default;
	~DesktopInstance();

	// ISurface
	Span<std::string_view> vkInstanceExtensions() const override;
	bool vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const override;
	ErasedRef nativePtr() const noexcept override;

	// IInstance
	EventQueue pollEvents() override;

	// Desktop
	glm::ivec2 windowSize() const noexcept;
	glm::ivec2 framebufferSize() const noexcept;
	CursorType cursorType() const noexcept;
	CursorMode cursorMode() const noexcept;
	glm::vec2 cursorPosition() const noexcept;

	void cursorType(CursorType type);
	void cursorMode(CursorMode mode);
	void cursorPosition(glm::vec2 position);

	void show();
	void close();

	bool importControllerDB(std::string_view db) const;
	std::vector<Gamepad> activeGamepads() const;
	Joystick joyState(s32 id) const;
	Gamepad gamepadState(s32 id) const;
	f32 triggerToAxis(f32 triggerValue) const;
	std::size_t joystickAxesCount(s32 id) const;
	std::size_t joysticKButtonsCount(s32 id) const;
	std::string_view toString(s32 key) const;
};

struct DesktopInstance::CreateInfo {
	struct {
		std::string title;
		glm::ivec2 size = {32, 32};
		glm::ivec2 centreOffset = {};
	} config;

	struct {
		LibLogger::Verbosity verbosity = LibLogger::Verbosity::eLibUser;
		Style style = Style::eDecoratedWindow;
		u8 screenID = 0;
		bool bCentreCursor = true;
		bool bAutoShow = false;
	} options;
};
} // namespace le::window
#endif
