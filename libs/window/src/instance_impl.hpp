#pragma once
#include <window/instance.hpp>

#if defined(LEVK_USE_GLFW)
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <core/not_null.hpp>
#include <ktl/fixed_any.hpp>

namespace le::window {
struct Cursor {
	ktl::fixed_any<> data;
	CursorType type;
};

#if defined(LEVK_USE_GLFW)
inline std::unordered_map<GLFWwindow*, not_null<Instance::Impl*>> g_impls;
#endif

class Manager::Impl {
#if defined(LEVK_USE_GLFW)
  public:
	Impl(not_null<LibLogger*> logger);
	~Impl();

	Cursor const& cursor(CursorType type);
	LibLogger& log() noexcept { return *m_logger; }
	GLFWwindow* make(CreateInfo const& info);

	Span<GLFWmonitor* const> displays() const;

  private:
	EnumArray<CursorType, Cursor> m_cursors;
	not_null<LibLogger*> m_logger;

	friend class Manager;
#endif
};

class Instance::Impl {
  public:
#if defined(LEVK_USE_GLFW)
	Impl(not_null<Manager::Impl*> manager, not_null<GLFWwindow*> win);
	~Impl();
#endif

	LibLogger& log() noexcept { return m_manager->log(); }

	EventQueue pollEvents();
	bool show();
	bool hide();
	bool visible() const noexcept;
	void close();
	bool closing() const noexcept;

	glm::ivec2 position() const noexcept;
	void position(glm::ivec2 pos) noexcept;
	void maximize() noexcept;
	void restore() noexcept;

	CursorMode cursorMode() const noexcept;
	void cursorType(CursorType type);
	void cursorMode([[maybe_unused]] CursorMode mode);
	glm::vec2 cursorPosition() const noexcept;
	glm::uvec2 windowSize() const noexcept;
	glm::uvec2 framebufferSize() const noexcept;
	bool importControllerDB(std::string_view db) const;
	ktl::fixed_vector<Gamepad, 8> activeGamepads() const;
	Joystick joyState(s32 id) const;
	Gamepad gamepadState(s32 id) const;
	std::size_t joystickAxesCount(s32 id) const;
	std::size_t joysticKButtonsCount(s32 id) const;

	Cursor m_active;
	bool m_maximized{};
#if defined(LEVK_USE_GLFW)
	not_null<GLFWwindow*> m_win;
#endif

  private:
#if defined(LEVK_USE_GLFW)
	static void onFocus(GLFWwindow* win, int entered);
	static void onWindowResize(GLFWwindow* win, int width, int height);
	static void onFramebufferResize(GLFWwindow* win, int width, int height);
	static void onIconify(GLFWwindow* win, int iconified);
	static void onClose(GLFWwindow* win);
	static void onKey(GLFWwindow* win, int key, int scancode, int action, int mods);
	static void onMouse(GLFWwindow* win, f64 x, f64 y);
	static void onMouseButton(GLFWwindow* win, int key, int action, int mods);
	static void onText(GLFWwindow* win, u32 codepoint);
	static void onScroll(GLFWwindow* win, f64 dx, f64 dy);
	static void onMaximize(GLFWwindow* win, int maximized);
#endif

	EventQueue m_events;
	not_null<Manager::Impl*> m_manager;
};
} // namespace le::window
#endif
