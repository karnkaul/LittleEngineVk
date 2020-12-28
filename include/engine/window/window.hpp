#pragma once
#include <array>
#include <memory>
#include <string>
#include <engine/gfx/render_driver.hpp>
#include <engine/window/common.hpp>
#include <engine/window/input_types.hpp>
#include <glm/glm.hpp>

namespace le {
class Window final {
  public:
	enum class Mode { eDecoratedWindow = 0, eBorderlessWindow, eBorderlessFullscreen, eDedicatedFullscreen, eCOUNT_ };

	inline static EnumArray<Mode> const s_modeNames = {"Decorated Window", "Borderless Window", "Borderless Fullscreen", "Dedicated Fullscreen"};

	struct Info final {
		struct {
			std::string title;
			glm::ivec2 size = {32, 32};
			glm::ivec2 centreOffset = {};
			Mode mode = Mode::eDecoratedWindow;
			u8 virtualFrameCount = 3;
		} config;

		struct {
			PriorityList<ColourSpace> colourSpaces;
			PriorityList<PresentMode> presentModes;
			u8 screenID = 0;
			bool bCentreCursor = true;
		} options;
	};

	struct Service final {
		Service();
		~Service();
	};

  public:
	static const std::string s_tName;

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

	// Pass WindowID::null for global registration
	[[nodiscard]] static input::OnText::Tk registerText(input::OnText::Callback callback, WindowID window);
	// Pass WindowID::null for global registration
	[[nodiscard]] static input::OnInput::Tk registerInput(input::OnInput::Callback callback, WindowID window);
	// Pass WindowID::null for global registration
	[[nodiscard]] static input::OnMouse::Tk registerMouse(input::OnMouse::Callback callback, WindowID window);
	// Pass WindowID::null for global registration
	[[nodiscard]] static input::OnMouse::Tk registerScroll(input::OnMouse::Callback callback, WindowID window);

	static WindowID editorWindow();

  public:
	gfx::render::Driver const& driver() const;
	gfx::render::Driver& driver();

	WindowID id() const;
	bool open() const;
	bool exists() const;
	bool closing() const;
	bool focused() const;

	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

  public:
	[[nodiscard]] input::OnText::Tk registerText(input::OnText::Callback callback);
	// Callback parameters: (Key key, Action action, Mods mods)
	[[nodiscard]] input::OnInput::Tk registerInput(input::OnInput::Callback callback);
	// Callback parameters: (f64 x, f64 y)
	[[nodiscard]] input::OnMouse::Tk registerMouse(input::OnMouse::Callback callback);
	// Callback parameters: (f32 dx, f32 dy)
	[[nodiscard]] input::OnMouse::Tk registerScroll(input::OnMouse::Callback callback);
	// Callback parameters: (io::Path filepath)
	[[nodiscard]] input::OnFiledrop::Tk registerFiledrop(input::OnFiledrop::Callback callback);
	// Callback parameters: (bool bInFocus)
	[[nodiscard]] input::OnFocus::Tk registerFocus(input::OnFocus::Callback callback);
	// Callback parameters: (s32 x, s32 y)
	[[nodiscard]] input::OnWindowResize::Tk registerResize(input::OnWindowResize::Callback callback);
	[[nodiscard]] input::OnClosed::Tk registerClosed(input::OnClosed::Callback callback);

  public:
	bool create(Info const& info);
	void setClosing();
	void destroy();

	PresentMode presentMode() const;
	std::vector<PresentMode> supportedPresentModes() const;
	bool setPresentMode(PresentMode mode);

	void setCursorType(input::CursorType type) const;
	void setCursorMode(input::CursorMode mode) const;
	input::CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos) const;
	std::string clipboard() const;

  private:
	friend class WindowImpl;

  private:
	gfx::render::Driver m_driver;
	std::unique_ptr<class WindowImpl> m_uImpl;
	WindowID m_id = WindowID::null;
};
} // namespace  le
