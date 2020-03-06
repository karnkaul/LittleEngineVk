#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "core/zero.hpp"
#include "input_types.hpp"

namespace le
{
class Window final
{
public:
	using ID = TZero<s32, -1>;

public:
	enum class Mode
	{
		DecoratedWindow = 0,
		BorderlessWindow,
		BorderlessFullscreen,
		DedicatedFullscreen,
		COUNT_
	};

	struct Data final
	{
		std::string title;
		glm::ivec2 size = {};
		glm::ivec2 position = {};
		u8 screenID = 0;
		Mode mode = Mode::DecoratedWindow;
		bool bCentreCursor = true;
	};

	struct Service final
	{
		Service();
		~Service();
	};
	
public:
	static const std::string s_tName;

private:
	std::unique_ptr<class WindowImpl> m_uImpl;
	ID m_id = ID::Null;

public:
	Window();
	Window(Window&&);
	Window& operator=(Window&&);
	~Window();

public:
	static void pollEvents();

public:
	ID id() const;
	bool isOpen() const;
	bool isClosing() const;

public:
	bool create(Data const& data);
	void close();
	void destroy();

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
	[[nodiscard]] OnResize::Token registerResize(OnResize::Callback callback);
	[[nodiscard]] OnClosed::Token registerClosed(OnClosed::Callback callback);

	void setCursorMode(CursorMode mode) const;
	CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos) const;
	std::string getClipboard() const;

	static JoyState getJoyState(s32 id);
	static GamepadState getGamepadState(s32 id);
	static f32 triggerToAxis(f32 triggerValue);
	static size_t joystickAxesCount(s32 id);
	static size_t joysticKButtonsCount(s32 id);

	static std::string_view toString(s32 key);

private:
	friend class WindowImpl;
};
} // namespace  le
