#include <detail/window_helper.hpp>
#include <levk/core/error.hpp>
#include <levk/core/is_positive.hpp>
#include <levk/core/thread_index.hpp>
#include <levk/engine.hpp>
#include <levk/input/state.hpp>
#include <levk/logger.hpp>
#include <cmath>

namespace levk {
namespace {
auto noop_event_sink{EventSink{}}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class Engine : public IEngine, public IEventSink {
  public:
	using CreateInfo = EngineCreateInfo;

	explicit Engine(CreateInfo const& create_info) : m_event_sink(&noop_event_sink) {
		thread::initialize();
		m_window = make_window(create_info);
		set_window_callbacks();

		auto const rdci = RenderDeviceCreateInfo{
			.window = m_window.get(),
			.app_name = create_info.app_name,
			.app_version = create_info.app_version,
			.gpu_selector = create_info.gpu_selector,
		};
		m_render_device = create_render_device(rdci);
	}

  private:
	[[nodiscard]] auto get_cursor_position() const -> glm::ivec2 final { return window_to_framebuffer(detail::get_cursor_position(m_window.get())); }

	[[nodiscard]] auto get_window_size() const -> glm::ivec2 final { return detail::get_window_size(m_window.get()); }

	[[nodiscard]] auto get_framebuffer_size() const -> glm::ivec2 final { return detail::get_framebuffer_size(m_window.get()); }

	[[nodiscard]] auto get_window() const -> Ptr<GLFWwindow> final { return m_window.get(); }
	[[nodiscard]] auto is_window_open() const -> bool final { return glfwWindowShouldClose(m_window.get()) == GLFW_FALSE; }

	void toggle_window(bool show) final {
		if (show) {
			glfwShowWindow(m_window.get());
		} else {
			glfwHideWindow(m_window.get());
		}
	}

	void shutdown() final { glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE); }

	[[nodiscard]] auto get_render_device() const -> IRenderDevice& final { return *m_render_device; }

	void poll_events(IEventSink& event_sink) final {
		m_event_sink = &event_sink;
		pre_poll();
		glfwPollEvents();
		post_poll();
		m_event_sink = &noop_event_sink;
	}

	[[nodiscard]] auto get_input_state() const -> input::State const& final { return m_input_state; }

	void on_focus(bool const in_focus) final { m_event_sink->on_focus(in_focus); }

	auto on_close() -> bool override {
		auto const ret = m_event_sink->on_close();
		if (!ret) { glfwSetWindowShouldClose(m_window.get(), GLFW_FALSE); }
		return ret;
	}

	void on_window_resize(glm::vec2 const window_size) final { m_event_sink->on_window_resize(window_size); }

	void on_framebuffer_resize(glm::vec2 const framebuffer_size) final { m_event_sink->on_framebuffer_resize(framebuffer_size); }

	void on_key_down(int const key, int const mods) final {
		auto const index = input::to_safe_index<input::max_keys_v>(key);
		if (index) { m_input_state.keys.table[*index] = input::Action::ePress; } // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		m_event_sink->on_key_down(key, mods);
	}

	void on_key_up(int const key, int const mods) final {
		auto const index = input::to_safe_index<input::max_keys_v>(key);
		if (index) { m_input_state.keys.table[*index] = input::Action::eRelease; } // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		m_event_sink->on_key_up(key, mods);
	}

	void on_key_repeat(int const key, int const mods) final {
		auto const index = input::to_safe_index<input::max_keys_v>(key);
		if (index) { m_input_state.keys.table[*index] = input::Action::eHold; } // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		m_event_sink->on_key_repeat(key, mods);
	}

	void on_char(std::uint32_t const code) final { m_event_sink->on_char(code); }

	void on_mouse_down(glm::ivec2 const position, int const button, int const mods) final {
		m_input_state.cursor_position = position;
		auto const index = input::to_safe_index<input::max_mouse_buttons_v>(button);
		if (index) { m_input_state.mouse_buttons.table[*index] = input::Action::ePress; } // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		m_event_sink->on_mouse_down(position, button, mods);
	}

	void on_mouse_up(glm::ivec2 const position, int const button, int const mods) final {
		m_input_state.cursor_position = position;
		auto const index = input::to_safe_index<input::max_mouse_buttons_v>(button);
		if (index) { m_input_state.mouse_buttons.table[*index] = input::Action::eRelease; } // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		m_event_sink->on_mouse_up(position, button, mods);
	}

	void on_cursor_move(glm::ivec2 const position) final {
		m_input_state.cursor_position = position;
		m_event_sink->on_cursor_move(position);
	}

	void on_scroll_wheel(glm::vec2 const scroll) final {
		m_input_state.mouse_scroll = scroll;
		m_event_sink->on_scroll_wheel(scroll);
	}

	void on_file_drop(std::span<char const*> paths) final { m_event_sink->on_file_drop(paths); }

	struct Deleter {
		void operator()(Ptr<GLFWwindow> window) const noexcept {
			glfwDestroyWindow(window);
			glfwTerminate();
		}
	};

	[[nodiscard]] static auto make_window(CreateInfo const& create_info) -> std::unique_ptr<GLFWwindow, Deleter> {
		if (glfwInit() != GLFW_TRUE) { throw Error{"Failed to initialize GLFW"}; }
		if (glfwVulkanSupported() != GLFW_TRUE) {
			glfwTerminate();
			throw Error{"Vulkan not supported"};
		}
		auto ret = std::unique_ptr<GLFWwindow, Deleter>{};
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		if (!create_info.show_window_immediately) { glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); }
		ret.reset(glfwCreateWindow(create_info.window_size.x, create_info.window_size.y, create_info.window_title.c_str(), nullptr, nullptr));
		if (!ret) { throw Error{"Failed to create Window"}; }
		return ret;
	}

	[[nodiscard]] static auto self(Ptr<GLFWwindow> window) -> Engine& {
		auto* ptr = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		if (ptr == nullptr) { throw Error{"Window user pointer is null"}; }
		return *ptr;
	}

	[[nodiscard]] auto window_to_framebuffer(glm::dvec2 position) const -> glm::ivec2 {
		glm::dvec2 const fb_size = get_framebuffer_size();
		glm::dvec2 const w_size = get_framebuffer_size();
		if (!is_positive(w_size) || !is_positive(fb_size)) { return {}; }
		auto ndc = position / w_size;
		ndc.x = ndc.x - 0.5f;
		ndc.y = 0.5f - ndc.y;
		auto const ret = ndc * fb_size;
		return {std::floor(ret.x), std::floor(ret.y)};
	}

	void set_window_callbacks() {
		glfwSetWindowUserPointer(m_window.get(), this);
		glfwSetWindowCloseCallback(m_window.get(), [](Ptr<GLFWwindow> window) { self(window).on_close(); });
		glfwSetWindowFocusCallback(m_window.get(), [](Ptr<GLFWwindow> window, int v) { self(window).on_focus(v == GLFW_TRUE); });
		glfwSetWindowSizeCallback(m_window.get(), [](Ptr<GLFWwindow> window, int x, int y) { self(window).on_window_resize({x, y}); });
		glfwSetFramebufferSizeCallback(m_window.get(), [](Ptr<GLFWwindow> window, int x, int y) { self(window).on_framebuffer_resize({x, y}); });
		glfwSetKeyCallback(m_window.get(), [](Ptr<GLFWwindow> window, int key, int /*scancode*/, int action, int mods) {
			auto& eng = self(window);
			switch (action) {
			case GLFW_PRESS: eng.on_key_down(key, mods); break;
			case GLFW_RELEASE: eng.on_key_up(key, mods); break;
			case GLFW_REPEAT: eng.on_key_repeat(key, mods); break;
			default: break;
			}
		});
		glfwSetCharCallback(m_window.get(), [](Ptr<GLFWwindow> window, std::uint32_t code) { self(window).on_char(code); });
		glfwSetCursorPosCallback(m_window.get(),
								 [](Ptr<GLFWwindow> window, double x, double y) { self(window).on_cursor_move(self(window).window_to_framebuffer({x, y})); });
		glfwSetScrollCallback(m_window.get(), [](Ptr<GLFWwindow> window, double x, double y) { self(window).on_scroll_wheel({x, y}); });
		glfwSetMouseButtonCallback(m_window.get(), [](Ptr<GLFWwindow> window, int button, int action, int mods) {
			auto& eng = self(window);
			switch (action) {
			case GLFW_PRESS: eng.on_mouse_down(eng.get_cursor_position(), button, mods); break;
			case GLFW_RELEASE: eng.on_mouse_up(eng.get_cursor_position(), button, mods); break;
			default: break;
			}
		});
		glfwSetDropCallback(m_window.get(), [](Ptr<GLFWwindow> window, int count, char const* paths[]) { // NOLINT
			self(window).on_file_drop({paths, static_cast<std::size_t>(count)});
		});
	}

	void pre_poll() {
		m_input_state.mouse_scroll = {};
		auto const update_table = [](auto& table) {
			for (auto& action : table) {
				switch (action) {
				case input::Action::ePress: action = input::Action::eHold; break;
				case input::Action::eRelease: action = input::Action::eNone; break;
				default: break;
				}
			}
		};
		update_table(m_input_state.keys.table);
		update_table(m_input_state.mouse_buttons.table);
	}

	void post_poll() { m_render_device->new_frame(); }

	Logger m_log{"Engine"};

	std::unique_ptr<GLFWwindow, Deleter> m_window{};
	std::unique_ptr<IRenderDevice> m_render_device{};
	input::State m_input_state{};

	NotNull<IEventSink*> m_event_sink;
};
} // namespace
} // namespace levk

auto levk::create_engine(EngineCreateInfo const& create_info) -> std::unique_ptr<IEngine> { return std::make_unique<Engine>(create_info); }
