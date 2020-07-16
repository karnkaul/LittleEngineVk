#pragma once
#include <array>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <engine/window/input_types.hpp>
#include <engine/window/common.hpp>
#include <engine/gfx/renderer.hpp>

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

	static std::array<std::string_view, (std::size_t)Mode::eCOUNT_> const s_modeNames;

	struct Info final
	{
		struct
		{
			std::string title;
			glm::ivec2 size = {32, 32};
			glm::ivec2 centreOffset = {};
			Mode mode = Mode::eDecoratedWindow;
			u8 virtualFrameCount = 3;
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
	static bool anyActive();
	static bool anyExist();
	static void pollEvents();
	static void renderAll();

	// Pass WindowID::s_null for global registration
	[[nodiscard]] static input::OnText::Token registerText(input::OnText::Callback callback, WindowID window);
	// Pass WindowID::s_null for global registration
	[[nodiscard]] static input::OnInput::Token registerInput(input::OnInput::Callback callback, WindowID window);
	// Pass WindowID::s_null for global registration
	[[nodiscard]] static input::OnMouse::Token registerMouse(input::OnMouse::Callback callback, WindowID window);
	// Pass WindowID::s_null for global registration
	[[nodiscard]] static input::OnMouse::Token registerScroll(input::OnMouse::Callback callback, WindowID window);

	static WindowID editorWindow();

public:
	gfx::Renderer const& renderer() const;
	gfx::Renderer& renderer();

	WindowID id() const;
	bool isOpen() const;
	bool exists() const;
	bool isClosing() const;
	bool isFocused() const;

	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

public:
	[[nodiscard]] input::OnText::Token registerText(input::OnText::Callback callback);
	// Callback parameters: (Key key, Action action, Mods mods)
	[[nodiscard]] input::OnInput::Token registerInput(input::OnInput::Callback callback);
	// Callback parameters: (f64 x, f64 y)
	[[nodiscard]] input::OnMouse::Token registerMouse(input::OnMouse::Callback callback);
	// Callback parameters: (f32 dx, f32 dy)
	[[nodiscard]] input::OnMouse::Token registerScroll(input::OnMouse::Callback callback);
	// Callback parameters: (std::filesystem::path filepath)
	[[nodiscard]] input::OnFiledrop::Token registerFiledrop(input::OnFiledrop::Callback callback);
	// Callback parameters: (bool bInFocus)
	[[nodiscard]] input::OnFocus::Token registerFocus(input::OnFocus::Callback callback);
	// Callback parameters: (s32 x, s32 y)
	[[nodiscard]] input::OnWindowResize::Token registerResize(input::OnWindowResize::Callback callback);
	[[nodiscard]] input::OnClosed::Token registerClosed(input::OnClosed::Callback callback);

public:
	bool create(Info const& info);
	void close();
	void destroy();

	PresentMode presentMode() const;
	std::vector<PresentMode> supportedPresentModes() const;
	bool setPresentMode(PresentMode mode);

	void setCursorMode(input::CursorMode mode) const;
	input::CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos) const;
	std::string clipboard() const;

private:
	friend class WindowImpl;
};
} // namespace  le
