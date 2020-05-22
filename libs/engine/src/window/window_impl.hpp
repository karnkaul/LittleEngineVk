#pragma once
#include <memory>
#include <unordered_set>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "engine/window/window.hpp"

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
		OnText onText;
		OnInput onInput;
		OnMouse onMouse;
		OnMouse onScroll;
		OnFiledrop onFiledrop;
		OnFocus onFocus;
		OnWindowResize onWindowResize;
		OnClosed onClosed;
	};

	static std::unordered_map<WindowID::type, InputCallbacks> s_input;

	glm::ivec2 m_windowSize = {};
	glm::ivec2 m_framebufferSize = {};
	std::unique_ptr<NativeWindow> m_uNativeWindow;
	Window* m_pWindow;

	static bool init();
	static void deinit();
	static void updateActive();
	static std::vector<char const*> vulkanInstanceExtensions();
	static WindowImpl* windowImpl(WindowID window);
	static gfx::RendererImpl* rendererImpl(WindowID window);
	static std::unordered_set<s32> active();
	static vk::SurfaceKHR createSurface(vk::Instance instance, NativeWindow const& nativeWindow);
	static void* nativeHandle(WindowID window);
	static WindowID guiWindow();

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

	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

	void setCursorMode(CursorMode mode) const;
	CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos);
	std::string clipboard() const;
	static JoyState joyState(s32 id);
	static GamepadState gamepadState(s32 id);
	static f32 triggerToAxis(f32 triggerValue);
	static size_t joystickAxesCount(s32 id);
	static size_t joysticKButtonsCount(s32 id);
	static std::string_view toString(s32 key);
	static void pollEvents();
};
} // namespace le
