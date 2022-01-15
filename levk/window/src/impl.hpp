#pragma once
#include <levk/window/window.hpp>

#if defined(LEVK_USE_GLFW)
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <event_builder.hpp>
#include <ktl/fixed_any.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/utils/unique.hpp>
#include <unordered_map>

namespace le::window {
struct Cursor {
	ktl::fixed_any<> data;
	CursorType type;
};

#if defined(LEVK_USE_GLFW)
inline std::unordered_map<GLFWwindow*, not_null<Window::Impl*>> g_impls;
#endif

struct GlfwInst {
	EnumArray<CursorType, Cursor> cursors;
	bool init{};

	static std::optional<GlfwInst> make() noexcept;

	constexpr bool operator==(GlfwInst const& rhs) const { return init == rhs.init; }
};

struct GlfwDel {
	void operator()(GlfwInst const&) const noexcept;
	void operator()(GLFWwindow* win) const noexcept;
};

using UniqueGlfwInst = Unique<GlfwInst, GlfwDel>;
using UniqueGlfwWin = Unique<GLFWwindow*, GlfwDel>;

struct Manager::Impl {
#if defined(LEVK_USE_GLFW)
	Cursor const& cursor(CursorType type);
	UniqueGlfwWin make(CreateInfo const& info);

	Span<GLFWmonitor* const> displays() const;

	UniqueGlfwInst inst;
#endif
};

class Window::Impl {
  public:
#if defined(LEVK_USE_GLFW)
	Impl(not_null<Manager::Impl*> manager, UniqueGlfwWin win);
	~Impl() noexcept;
#endif

	Span<Event const> pollEvents();
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
	UniqueGlfwWin m_win;
	EventStorage m_eventStorage;
#endif

  private:
#if defined(LEVK_USE_GLFW)
	static void onClose(GLFWwindow* win);
	static void onFocus(GLFWwindow* win, int entered);
	static void onCursorEnter(GLFWwindow* win, int entered);
	static void onMaximize(GLFWwindow* win, int maximized);
	static void onIconify(GLFWwindow* win, int iconified);
	static void onPosition(GLFWwindow* win, int x, int y);
	static void onWindowResize(GLFWwindow* win, int width, int height);
	static void onFramebufferResize(GLFWwindow* win, int width, int height);
	static void onCursorPos(GLFWwindow* win, f64 x, f64 y);
	static void onScroll(GLFWwindow* win, f64 dx, f64 dy);
	static void onKey(GLFWwindow* win, int key, int scancode, int action, int mods);
	static void onMouseButton(GLFWwindow* win, int key, int action, int mods);
	static void onText(GLFWwindow* win, u32 codepoint);
	static void onFileDrop(GLFWwindow* win, int count, char const** paths);
#endif

	std::vector<Event> m_events;
	not_null<Manager::Impl*> m_manager;
};
} // namespace le::window
#endif
