#pragma once
#include <array>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "input_types.hpp"
#include "common.hpp"
#include "engine/gfx/renderer.hpp"

#define GUI(expr)                                  \
	if (le::Window::guiWindow() != le::WindowID()) \
	{                                              \
		expr;                                      \
	}

namespace le
{
class Window final
{
public:
	enum class Mode
	{
		eDecoratedWindow = 0,
		eBorderlessWindow,
		eBorderlessFullscreen,
		eDedicatedFullscreen,
		eCOUNT_
	};

	static std::array<std::string_view, (size_t)Mode::eCOUNT_> const s_modeNames;

	struct Info final
	{
		struct
		{
			std::string title;
			glm::ivec2 size = {32, 32};
			glm::ivec2 centreOffset = {};
			Mode mode = Mode::eDecoratedWindow;
			u8 virtualFrameCount = 3;
			bool bEnableGUI = false;
		} config;

		struct
		{
			PriorityList<ColourSpace> colourSpaces;
			PriorityList<PresentMode> presentModes;
			u8 screenID = 0;
			bool bCentreCursor = true;
		} options;
	};

	struct Service final
	{
		Service();
		~Service();
	};

public:
	static const std::string s_tName;

private:
	gfx::Renderer m_renderer;
	std::unique_ptr<class WindowImpl> m_uImpl;
	WindowID m_id = WindowID::s_null;

public:
	Window();
	Window(Window&&);
	Window& operator=(Window&&);
	~Window();

public:
	static void pollEvents();
	static void renderAll();

	// Pass WindowID::s_null for global registration
	[[nodiscard]] static OnText::Token registerText(OnText::Callback callback, WindowID window);
	// Pass WindowID::s_null for global registration
	[[nodiscard]] static OnInput::Token registerInput(OnInput::Callback callback, WindowID window);
	// Pass WindowID::s_null for global registration
	[[nodiscard]] static OnMouse::Token registerMouse(OnMouse::Callback callback, WindowID window);
	// Pass WindowID::s_null for global registration
	[[nodiscard]] static OnMouse::Token registerScroll(OnMouse::Callback callback, WindowID window);

	static WindowID guiWindow();

public:
	gfx::Renderer const& renderer() const;
	gfx::Renderer& renderer();

	WindowID id() const;
	bool isOpen() const;
	bool isClosing() const;
	bool isFocused() const;

	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

public:
	[[nodiscard]] OnText::Token registerText(OnText::Callback callback);
	// Callback parameters: (Key key, Action action, Mods mods)
	[[nodiscard]] OnInput::Token registerInput(OnInput::Callback callback);
	// Callback parameters: (f64 x, f64 y)
	[[nodiscard]] OnMouse::Token registerMouse(OnMouse::Callback callback);
	// Callback parameters: (f32 dx, f32 dy)
	[[nodiscard]] OnMouse::Token registerScroll(OnMouse::Callback callback);
	// Callback parameters: (std::filesystem::path filepath)
	[[nodiscard]] OnFiledrop::Token registerFiledrop(OnFiledrop::Callback callback);
	// Callback parameters: (bool bInFocus)
	[[nodiscard]] OnFocus::Token registerFocus(OnFocus::Callback callback);
	// Callback parameters: (s32 x, s32 y)
	[[nodiscard]] OnWindowResize::Token registerResize(OnWindowResize::Callback callback);
	[[nodiscard]] OnClosed::Token registerClosed(OnClosed::Callback callback);

public:
	bool create(Info const& info);
	void close();
	void destroy();

	void setCursorMode(CursorMode mode) const;
	CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos) const;
	std::string clipboard() const;

	static JoyState joyState(s32 id);
	static GamepadState gamepadState(s32 id);
	static f32 triggerToAxis(f32 triggerValue);
	static size_t joystickAxesCount(s32 id);
	static size_t joysticKButtonsCount(s32 id);

	static std::string_view toString(s32 key);

private:
	friend class WindowImpl;
};
} // namespace  le
