#include <impl.hpp>
#include <ktl/enum_flags/bitflags.hpp>
#include <levk/core/not_null.hpp>
#include <levk/window/window.hpp>
#include <unordered_map>

#if defined(LEVK_USE_GLFW)
#include <ktl/fixed_any.hpp>
#include <levk/core/array_map.hpp>
#include <levk/core/log.hpp>
#include <levk/core/utils/error.hpp>
#include <levk/core/utils/expect.hpp>
#endif

namespace le::window {
using lvl = LogLevel;
constexpr std::string_view g_name = "Window";

std::optional<Manager> Manager::make() {
#if defined(LEVK_USE_GLFW)
	auto inst = GlfwInst::make();
	EXPECT(inst);
	if (!inst) { return std::nullopt; }
	auto impl = std::make_unique<Impl>();
	impl->inst = std::move(*inst);
	Manager ret(std::move(impl));
	return std::move(ret);
#endif
}

Manager::Manager(std::unique_ptr<Impl>&& impl) noexcept : m_impl(std::move(impl)) {
	log(lvl::info, LC_LibUser, "[{}] Manager initialised successfully", g_name);
}
Manager::Manager(Manager&&) noexcept = default;
Manager& Manager::operator=(Manager&&) noexcept = default;
Manager::~Manager() noexcept = default;

std::optional<Window> Manager::makeWindow(CreateInfo const& info) {
	std::optional<Window> ret;
#if defined(LEVK_USE_GLFW)
	if (m_impl) {
		if (auto win = m_impl->make(info)) {
			ret = Window(std::make_unique<Window::Impl>(m_impl.get(), std::move(win)));
			ret->m_impl->m_maximized = info.config.maximized;
		}
	}
#endif
	return ret;
}

std::size_t Manager::displayCount() const {
	std::size_t ret{};
#if defined(LEVK_USE_GLFW)
	if (m_impl) { ret = m_impl->displays().size(); }
#endif
	return ret;
}

Window::Window(Window&&) noexcept = default;
Window& Window::operator=(Window&&) noexcept = default;
Window::~Window() noexcept = default;
Window::Window(std::unique_ptr<Impl>&& impl) noexcept : m_impl(std::move(impl)) {}
Span<Event const> Window::pollEvents() { return m_impl->pollEvents(); }
bool Window::show() { return m_impl->show(); }
bool Window::hide() { return m_impl->hide(); }
bool Window::visible() const noexcept { return m_impl->visible(); }
void Window::close() { m_impl->close(); }
bool Window::closing() const noexcept { return m_impl->closing(); }
glm::ivec2 Window::position() const noexcept { return m_impl->position(); }
void Window::position(glm::ivec2 pos) noexcept { m_impl->position(pos); }
bool Window::maximized() const noexcept { return m_impl->m_maximized; }
void Window::maximize() noexcept { m_impl->maximize(); }
void Window::restore() noexcept { m_impl->restore(); }
CursorType Window::cursorType() const noexcept { return m_impl->m_active.type; }
CursorMode Window::cursorMode() const noexcept { return m_impl->cursorMode(); }
void Window::cursorType(CursorType type) { m_impl->cursorType(type); }
void Window::cursorMode(CursorMode mode) { m_impl->cursorMode(mode); }
glm::vec2 Window::cursorPosition() const noexcept { return m_impl->cursorPosition(); }
glm::uvec2 Window::windowSize() const noexcept { return m_impl->windowSize(); }
glm::uvec2 Window::framebufferSize() const noexcept { return m_impl->framebufferSize(); }
bool Window::importControllerDB(std::string_view db) noexcept { return m_impl->importControllerDB(db); }
void Window::setIcon(Span<TBitmap<BmpView> const> bitmaps) { m_impl->setIcon(bitmaps); }
ktl::fixed_vector<Gamepad, 8> Window::activeGamepads() const noexcept { return m_impl->activeGamepads(); }
Joystick Window::joyState(s32 id) const noexcept { return m_impl->joyState(id); }
Gamepad Window::gamepadState(s32 id) const noexcept { return m_impl->gamepadState(id); }
std::size_t Window::joystickAxesCount(s32 id) const noexcept { return m_impl->joystickAxesCount(id); }
std::size_t Window::joysticKButtonsCount(s32 id) const noexcept { return m_impl->joysticKButtonsCount(id); }

std::string_view Window::keyName(int key, int scancode) noexcept {
	std::string_view ret = "(Unknown)";
#if defined(LEVK_USE_GLFW)
	switch (key) {
	case GLFW_KEY_SPACE: ret = "space"; break;
	case GLFW_KEY_LEFT_CONTROL: ret = "lctrl"; break;
	case GLFW_KEY_RIGHT_CONTROL: ret = "rctrl"; break;
	case GLFW_KEY_LEFT_SHIFT: ret = "lshift"; break;
	case GLFW_KEY_RIGHT_SHIFT: ret = "rshift"; break;
	default: {
		char const* const name = glfwGetKeyName(static_cast<int>(key), scancode);
		if (name) { ret = name; }
		break;
	}
	}
#endif
	return ret;
}
} // namespace le::window
