#pragma once
#include <memory>
#include <unordered_set>
#include <core/static_any.hpp>
#include <engine/window/window.hpp>
#include <window/native_window.hpp>

namespace le
{
namespace gfx
{
class Renderer;
class RendererImpl;
} // namespace gfx

class WindowImpl final
{
public:
	struct InputCallbacks
	{
		input::OnText onText;
		input::OnInput onInput;
		input::OnMouse onMouse;
		input::OnMouse onScroll;
		input::OnFiledrop onFiledrop;
		input::OnFocus onFocus;
		input::OnWindowResize onWindowResize;
		input::OnClosed onClosed;
	};

	static std::unordered_map<WindowID, InputCallbacks> s_input;

	glm::ivec2 m_windowSize = {};
	glm::ivec2 m_framebufferSize = {};
	std::unique_ptr<NativeWindow> m_uNativeWindow;
	std::vector<PresentMode> m_presentModes;
	Window* m_pWindow;
	struct
	{
		StaticAny<> data;
		input::CursorType type;
	} m_cursor;

	static WindowImpl* find(StaticAny<> nativeHandle);

	static bool init();
	static void deinit();
	static void update();
	static std::vector<char const*> vulkanInstanceExtensions();
	static WindowImpl* windowImpl(WindowID window);
	static gfx::RendererImpl* rendererImpl(WindowID window);
	static std::unordered_set<s32> allExisting();
	static StaticAny<> nativeHandle(WindowID window);
	static WindowID editorWindow();

	WindowImpl(Window* pWindow);
	~WindowImpl();

	bool create(Window::Info const& info);
	bool isOpen() const;
	bool exists() const;
	bool isClosing() const;
	bool isFocused() const;
	void close();
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
};
} // namespace le
