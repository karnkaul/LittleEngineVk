#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <impl/frame_profiler.hpp>
#include <le/core/enumerate.hpp>
#include <le/core/reversed.hpp>
#include <le/core/zip_ranges.hpp>
#include <le/engine.hpp>
#include <le/error.hpp>
#include <le/input/receiver.hpp>
#include <format>

namespace le {
namespace {
auto make_window(char const* title, glm::uvec2 extent) -> GLFWwindow* {
	if (glfwInit() != GLFW_TRUE || glfwVulkanSupported() != GLFW_TRUE) { throw Error{"Failed to initialize GLFW"}; }
	auto const size = glm::ivec2{extent};
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* ret = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
	if (ret == nullptr) { throw Error{"Failed to create GLFW window"}; }
	return ret;
}

constexpr auto to_action(int const input) -> input::Action {
	switch (input) {
	case GLFW_PRESS: return input::Action::ePress;
	case GLFW_RELEASE: return input::Action::eRelease;
	case GLFW_REPEAT: return input::Action::eRepeat;
	default: return input::Action::eNone;
	}
}

constexpr auto screen_to_world(glm::vec2 screen, glm::vec2 extent, glm::vec2 display_ratio) -> glm::vec2 {
	auto const he = 0.5f * glm::vec2{extent};
	auto ret = glm::vec2{screen.x - he.x, he.y - screen.y};
	auto const dr = display_ratio;
	if (dr.x == 0.0f || dr.y == 0.0f) { return ret; }
	return dr * ret;
}

auto g_input_state = input::State{};					// NOLINT
auto g_receivers = std::vector<Ptr<input::Receiver>>{}; // NOLINT

auto setup_signals(GLFWwindow* window) -> void {
	glfwSetWindowCloseCallback(window, [](GLFWwindow*) { g_input_state.shutting_down = true; });
	glfwSetWindowFocusCallback(window, [](GLFWwindow*, int focus) {
		g_input_state.in_focus = focus == GLFW_TRUE;
		g_input_state.changed |= input::State::eFocus;
	});
	glfwSetWindowSizeCallback(window, [](GLFWwindow*, int width, int height) {
		g_input_state.window_extent = glm::ivec2{width, height};
		g_input_state.changed |= input::State::eWindowSize;
	});
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
		g_input_state.framebuffer_extent = glm::ivec2{width, height};
		g_input_state.changed |= input::State::eFramebufferSize;
	});
	glfwSetCursorPosCallback(window, [](GLFWwindow*, double pos_x, double pos_y) {
		g_input_state.raw_cursor_position = glm::dvec2{pos_x, pos_y};
		g_input_state.changed |= input::State::eCursorPosition;
	});
	glfwSetKeyCallback(window, [](GLFWwindow*, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
		if (key >= 0 && key < static_cast<int>(g_input_state.keyboard.size())) { g_input_state.keyboard.at(static_cast<std::size_t>(key)) = to_action(action); }
		for (auto* receiver : reversed(g_receivers)) {
			if (receiver->on_key(key, action, mods)) { break; }
		}
	});
	glfwSetCharCallback(window, [](GLFWwindow*, std::uint32_t codepoint) {
		for (auto* receiver : reversed(g_receivers)) {
			if (receiver->on_char(graphics::Codepoint{codepoint})) { break; }
		}
	});
	glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, [[maybe_unused]] int mods) {
		g_input_state.mouse_buttons.at(static_cast<std::size_t>(button)) = to_action(action);
		for (auto* receiver : reversed(g_receivers)) {
			if (receiver->on_mouse(button, action, mods)) { break; }
		}
	});
	glfwSetDropCallback(window, [](GLFWwindow* /*window*/, int path_count, char const* paths[]) { // NOLINT
		auto const span = std::span{paths, static_cast<std::size_t>(path_count)};
		for (auto const* path : span) { g_input_state.drops.emplace_back(path); }
	});
}

auto update_gamepad(input::Gamepad& out, GLFWgamepadstate const& in) -> bool {
	bool ret{};
	for (auto const [glfw_button, gamepad_button] : zip_ranges(in.buttons, out.buttons)) {
		bool const is_press = glfw_button == GLFW_PRESS;
		if (is_press) { ret = true; }
		switch (gamepad_button) {
		case input::Action::eNone: {
			if (is_press) { gamepad_button = input::Action::ePress; }
			break;
		}
		case input::Action::eRelease: {
			if (is_press) {
				gamepad_button = input::Action::ePress;
			} else {
				gamepad_button = input::Action::eNone;
			}
			break;
		}
		case input::Action::ePress: {
			if (is_press) {
				gamepad_button = input::Action::eHold;
			} else {
				gamepad_button = input::Action::eRelease;
			}
			break;
		}
		case input::Action::eHold: {
			if (!is_press) { gamepad_button = input::Action::eRelease; }
			break;
		}
		default: break;
		}
	}

	out.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = in.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
	out.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = -in.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	out.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] = in.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
	out.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] = -in.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
	out.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = (in.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) * 0.5f;
	out.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = (in.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) * 0.5f;

	return ret;
}

auto update_gamepads() -> void {
	for (auto [gamepad, id] : enumerate<int>(g_input_state.gamepads)) {
		auto gamepad_state = GLFWgamepadstate{};
		if (gamepad.is_connected = glfwGetGamepadState(id, &gamepad_state) == GLFW_TRUE; !gamepad.is_connected) { continue; }
		if (update_gamepad(gamepad, gamepad_state)) { g_input_state.last_engaged_gamepad_index = static_cast<std::size_t>(id); }
	}
}
} // namespace

input::Receiver::Receiver() { g_receivers.push_back(this); }
input::Receiver::~Receiver() { std::erase(g_receivers, this); }

auto Engine::Deleter::operator()(GLFWwindow* ptr) const -> void {
	glfwDestroyWindow(ptr);
	glfwTerminate();
}

Engine::~Engine() {
	if (m_graphics_device) { m_graphics_device->get_device().waitIdle(); }
	m_resources->clear();
}

auto Engine::framebuffer_extent() const -> glm::uvec2 {
	auto ret = glm::ivec2{};
	glfwGetFramebufferSize(m_window.get(), &ret.x, &ret.y);
	return ret;
}

auto Engine::window_extent() const -> glm::uvec2 {
	auto ret = glm::ivec2{};
	glfwGetWindowSize(m_window.get(), &ret.x, &ret.y);
	return ret;
}

auto Engine::delta_time() const -> Duration { return m_stats.frame.time; }

auto Engine::input_state() const -> input::State const& { return g_input_state; } // NOLINT

auto Engine::frame_profile() const -> FrameProfile const& { return FrameProfiler::self().previous_profile(); } // NOLINT

auto Engine::next_frame() -> bool {
	FrameProfiler::self().start();

	update_stats();

	g_input_state.drops.clear();
	g_input_state.changed = {};
	auto advance = [](auto& action_array) {
		for (auto& action : action_array) {
			switch (action) {
			case input::Action::eRelease: action = input::Action::eNone; break;
			case input::Action::ePress:
			case input::Action::eHold:
			case input::Action::eRepeat: action = input::Action::eHold; break;
			default: break;
			}
		}
	};

	advance(g_input_state.keyboard);
	advance(g_input_state.mouse_buttons);
	update_gamepads();

	g_input_state.cursor_position = screen_to_world(g_input_state.raw_cursor_position, g_input_state.window_extent, g_input_state.display_ratio());

	glfwPollEvents();
	if (!is_running()) { return false; }
	if (m_image_index) { return true; }

	FrameProfiler::self().profile(FrameProfiler::Type::eAcquireFrame);
	m_image_index = m_renderer->wait_for_frame(framebuffer_extent());

	// There are very few points where we can recreate the swapchain without stalling the device.
	// This is NOT one of them, as although the renderer will itself have recreated the swapchain eg if it was out of date,
	// we already have an acquired image and signalled semaphore otherwise - aka we are in the middle of the swapchain loop.
	// Much more straightforward to finish the swapchain loop and THEN recreate it.
	// (This is why present mode requests are not handled here.)

	FrameProfiler::self().profile(FrameProfiler::Type::eTick);

	return true;
}

auto Engine::render(graphics::RenderFrame const& frame) -> void {
	if (!m_image_index) { return; }
	m_stats.frame.draw_calls = m_renderer->render(frame, *m_image_index);
	m_renderer->submit_frame(*m_image_index);
	m_image_index.reset();

	m_audio_device->set_transform(frame.camera->transform.position(), frame.camera->transform.orientation());

	if (m_queued_ops.present_mode) {
		// There are very few points where we can recreate the swapchain without stalling the device.
		// This is one of them, as the renderer will itself have recreated the swapchain eg if it was out of date.
		m_renderer->recreate_swapchain(*m_queued_ops.present_mode);
		m_queued_ops.present_mode.reset();
	}

	FrameProfiler::self().finish();
}

auto Engine::shutdown() -> void {
	glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE);
	g_input_state.shutting_down = true;
}

auto Engine::request_present_mode(vk::PresentModeKHR mode) -> bool {
	auto const supported = m_renderer->get_supported_present_modes();
	if (std::ranges::find(supported, mode) == supported.end()) { return false; }
	m_queued_ops.present_mode = mode;
	return true;
}

auto Engine::update_stats() -> void {
	m_stats.frame.time = m_delta_time();
	++m_fps.frames;
	m_fps.elapsed += m_stats.frame.time;
	if (m_fps.elapsed >= 1s) {
		m_stats.frame.rate = m_fps.frames;
		m_fps = {};
	}
	++m_stats.frame.count;

	auto const& pipeline_cache = graphics::PipelineCache::self();
	m_stats.cache.pipelines = static_cast<std::uint32_t>(pipeline_cache.pipeline_count());
	m_stats.cache.shaders = static_cast<std::uint32_t>(pipeline_cache.shader_count());
	m_stats.cache.vertex_buffers = static_cast<std::uint32_t>(graphics::VertexBufferCache::self().buffer_count());
}

auto Engine::Builder::set_extent(glm::uvec2 const value) -> Builder& {
	if (value.x > 0 && value.y > 0) { m_extent = value; }
	return *this;
}

auto Engine::Builder::set_shader_layout(graphics::ShaderLayout shader_layout) -> Builder& {
	m_shader_layout = std::move(shader_layout);
	return *this;
}

auto Engine::Builder::build() -> std::unique_ptr<Engine> {
	auto ret = std::make_unique<Engine>(ConstructTag{});

	ret->m_resources = std::make_unique<Resources>();

	ret->m_window = std::unique_ptr<GLFWwindow, Deleter>{make_window(m_title.c_str(), m_extent)};
	setup_signals(ret->m_window.get());

	auto rdci = graphics::Device::CreateInfo{
		.validation = debug_v,
	};
	ret->m_graphics_device = std::make_unique<graphics::Device>(ret->m_window.get(), rdci);

	auto framebuffer_extent = glm::ivec2{};
	glfwGetFramebufferSize(ret->m_window.get(), &framebuffer_extent.x, &framebuffer_extent.y);
	ret->m_renderer = std::make_unique<graphics::Renderer>(framebuffer_extent);
	auto imgui = std::make_unique<graphics::DearImGui>(ret->get_window(), ret->m_renderer->get_colour_format());
	ret->m_renderer->set_imgui(std::move(imgui));

	if (m_shader_layout) { graphics::PipelineCache::self().set_shader_layout(std::move(*m_shader_layout)); }

	ret->m_stats.gpu_name = ret->m_graphics_device->get_physical_device().getProperties().deviceName.data();
	ret->m_stats.validation_enabled = ret->m_graphics_device->get_info().validation;

	ret->m_audio_device = std::make_unique<audio::Device>();

	return ret;
}
} // namespace le
