#pragma once
#include <memory>
#include <unordered_set>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <engine/window/window.hpp>

namespace le
{
namespace gfx
{
class Renderer;
class RendererImpl;
} // namespace gfx

class NativeWindow final
{
public:
#if defined(LEVK_USE_GLFW)
	struct GLFWwindow* m_pWindow = nullptr;
#endif
	glm::ivec2 m_initialCentre = {};

public:
	NativeWindow(Window::Info const& info);
	~NativeWindow();

public:
	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;
};

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

	static std::unordered_map<WindowID::type, InputCallbacks> s_input;

	glm::ivec2 m_windowSize = {};
	glm::ivec2 m_framebufferSize = {};
	std::unique_ptr<NativeWindow> m_uNativeWindow;
	std::vector<PresentMode> m_presentModes;
	Window* m_pWindow;

	static bool init();
	static void deinit();
	static void update();
	static std::vector<char const*> vulkanInstanceExtensions();
	static WindowImpl* windowImpl(WindowID window);
	static gfx::RendererImpl* rendererImpl(WindowID window);
	static std::unordered_set<s32> active();
	static vk::SurfaceKHR createSurface(vk::Instance instance, NativeWindow const& nativeWindow);
	static void* nativeHandle(WindowID window);
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

	void setCursorMode(input::CursorMode mode) const;
	input::CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos);
	std::string clipboard() const;
	static input::Joystick joyState(s32 id);
	static input::Gamepad gamepadState(s32 id);
	static std::vector<input::Gamepad> activeGamepads();
	static f32 triggerToAxis(f32 triggerValue);
	static size_t joystickAxesCount(s32 id);
	static size_t joysticKButtonsCount(s32 id);
	static std::string_view toString(s32 key);

	static bool anyActive();
	static void pollEvents();
	static void renderAll();
};
} // namespace le
