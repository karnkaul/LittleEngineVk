#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "engine/window/window.hpp"

namespace le
{
namespace vuk
{
class Context;
class Instance;
} // namespace vuk

class NativeWindow final
{
public:
#if defined(LEVK_USE_GLFW)
	struct GLFWwindow* m_pWindow = nullptr;
#endif
	glm::ivec2 m_size = {};
	glm::ivec2 m_initialCentre = {};

public:
	NativeWindow(Window::Data const& data);
	~NativeWindow();
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
	glm::ivec2 m_size = {};
	std::unique_ptr<NativeWindow> m_uNativeWindow;
	std::unique_ptr<vuk::Context> m_uContext;
	Window* m_pWindow;

	static bool init();
	static void deinit();
	static std::vector<char const*> vulkanInstanceExtensions();
	static vuk::Context* context(WindowID window);

	WindowImpl(Window* pWindow);
	~WindowImpl();

	bool create(Window::Data const& data);
	bool isOpen() const;
	bool exists() const;
	bool isClosing() const;
	void close();
	void destroy();

	static vk::SurfaceKHR createSurface(vk::Instance instance, NativeWindow const& nativeWindow);
	glm::ivec2 framebufferSize();
	void onFramebufferSize(glm::ivec2 const& size);

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
