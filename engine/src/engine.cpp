#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spaced/engine.hpp>
#include <spaced/error.hpp>
#include <format>

namespace spaced {
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

// NOLINTNEXTLINE
auto g_input_state = input::State{};

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
		g_input_state.keyboard.at(static_cast<std::size_t>(key)) = to_action(action);
	});
	glfwSetCharCallback(window, [](GLFWwindow*, std::uint32_t codepoint) { g_input_state.characters.push_back(codepoint); });
	glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, [[maybe_unused]] int mods) {
		g_input_state.mouse_buttons.at(static_cast<std::size_t>(button)) = to_action(action);
	});
}
} // namespace

auto Engine::Deleter::operator()(GLFWwindow* ptr) const -> void {
	glfwDestroyWindow(ptr);
	glfwTerminate();
}

Engine::~Engine() {
	if (m_graphics_device) { m_graphics_device->device().waitIdle(); }
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

// NOLINTNEXTLINE
auto Engine::input_state() const -> input::State const& { return g_input_state; }

auto Engine::next_frame() -> bool {
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

	g_input_state.cursor_position = screen_to_world(g_input_state.raw_cursor_position, g_input_state.window_extent, g_input_state.display_ratio());

	glfwPollEvents();
	if (!is_running()) { return false; }
	if (m_image_index) { return true; }
	m_image_index = m_renderer->wait_for_frame(framebuffer_extent());
	return true;
}

auto Engine::render(std::span<NotNull<graphics::Subpass*> const> passes) -> void {
	if (!m_image_index) { return; }
	m_renderer->render(passes, *m_image_index);
	m_renderer->submit_frame(*m_image_index);
	m_image_index.reset();
}

auto Engine::shutdown() -> void {
	glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE);
	g_input_state.shutting_down = true;
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
	auto imgui = std::make_unique<graphics::DearImGui>(ret->window(), ret->m_renderer->get_colour_format(), ret->m_renderer->get_depth_format());
	ret->m_renderer->set_imgui(std::move(imgui));

	if (m_shader_layout) { graphics::PipelineCache::self().set_shader_layout(std::move(*m_shader_layout)); }

	return ret;
}
} // namespace spaced
