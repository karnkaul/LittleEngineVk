#pragma once
#include <unordered_set>
#include <core/erased_ref.hpp>
#include <core/static_any.hpp>
#include <core/utils/std_hash.hpp>
#include <engine/gfx/render_driver.hpp>
#include <engine/window/window.hpp>
#include <window/native_window.hpp>

namespace le {
class WindowImpl final {
  public:
	struct InputCallbacks {
		input::OnText onText;
		input::OnInput onInput;
		input::OnMouse onMouse;
		input::OnMouse onScroll;
		input::OnFiledrop onFiledrop;
		input::OnFocus onFocus;
		input::OnWindowResize onWindowResize;
		input::OnClosed onClosed;
	};
	struct Cursor {
		StaticAny<> data;
		input::CursorType type;
	};

	inline static std::unordered_map<WindowID, InputCallbacks> s_input;
	inline static EnumArray<input::CursorType, Cursor> s_cursors;

	glm::ivec2 m_windowSize = {};
	glm::ivec2 m_framebufferSize = {};
	NativeWindow m_nativeWindow;
	std::vector<PresentMode> m_presentModes;
	Cursor m_cursor;
	Window* m_pWindow;

	static WindowImpl* find(StaticAny<> nativeHandle);

	static bool init();
	static void deinit();
	static void update();
	static View<char const*> vulkanInstanceExtensions();
	static WindowImpl* windowImpl(WindowID window);
	static gfx::render::Driver::Impl* driverImpl(WindowID window);
	static std::unordered_set<s32> allExisting();
	static ErasedRef nativeHandle(WindowID window);
	static WindowID editorWindow();

	WindowImpl(Window* pWindow);
	~WindowImpl();

	bool create(Window* pWindow, Window::Info const& info);
	bool open() const;
	bool exists() const;
	bool closing() const;
	bool focused() const;
	void setClosing();
	void destroy();

	void onFramebufferSize(glm::ivec2 const& size);

	PresentMode presentMode() const;
	bool setPresentMode(PresentMode mode);

	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

	void setCursorType(input::CursorType type);
	void setCursorMode(input::CursorMode mode) const;
	input::CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos);
	std::string clipboard() const;

	static bool anyActive();
	static bool anyExist();
	static void pollEvents();
	static void renderAll();

	static bool importControllerDB(std::string_view db);
};
} // namespace le
