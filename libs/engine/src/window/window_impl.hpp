#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "engine/window/window.hpp"

namespace le
{
namespace gfx
{
class Presenter;
class Instance;
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

	InputCallbacks m_input;
	glm::ivec2 m_windowSize = {};
	glm::ivec2 m_framebufferSize = {};
	std::unique_ptr<NativeWindow> m_uNativeWindow;
	std::unique_ptr<gfx::Presenter> m_uPresenter;
	Window* m_pWindow;

	static bool init();
	static void deinit();
	static std::vector<char const*> vulkanInstanceExtensions();
	static gfx::Presenter* presenter(WindowID window);

	WindowImpl(Window* pWindow);
	~WindowImpl();

	bool create(Window::Info const& info);
	bool isOpen() const;
	bool exists() const;
	bool isClosing() const;
	void close();
	void destroy();

	static vk::SurfaceKHR createSurface(vk::Instance instance, NativeWindow const& nativeWindow);
	void onFramebufferSize(glm::ivec2 const& size);
	glm::ivec2 windowSize() const;
	glm::ivec2 framebufferSize() const;

	void setCursorMode(CursorMode mode) const;
	CursorMode cursorMode() const;
	glm::vec2 cursorPos() const;
	void setCursorPos(glm::vec2 const& pos);
	std::string getClipboard() const;
	static JoyState getJoyState(s32 id);
	static GamepadState getGamepadState(s32 id);
	static f32 triggerToAxis(f32 triggerValue);
	static size_t joystickAxesCount(s32 id);
	static size_t joysticKButtonsCount(s32 id);
	static std::string_view toString(s32 key);
	static void pollEvents();
};
} // namespace le
