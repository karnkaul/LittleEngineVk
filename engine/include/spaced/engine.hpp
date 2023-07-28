#pragma once
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <spaced/build_version.hpp>
#include <spaced/graphics/device.hpp>
#include <spaced/graphics/renderer.hpp>
#include <spaced/input/state.hpp>
#include <spaced/resources/resources.hpp>
#include <spaced/vfs/vfs.hpp>
#include <memory>
#include <optional>
#include <string>

namespace spaced {
class Engine : public MonoInstance<Engine> {
	struct ConstructTag {};

  public:
	class Builder;

	Engine(Engine const&) = delete;
	Engine(Engine&&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	auto operator=(Engine&&) -> Engine& = delete;

	~Engine();

	[[nodiscard]] auto window() const -> GLFWwindow const* { return m_window.get(); }
	[[nodiscard]] auto window() -> GLFWwindow* { return m_window.get(); }

	[[nodiscard]] auto framebuffer_extent() const -> glm::uvec2;
	[[nodiscard]] auto window_extent() const -> glm::uvec2;

	[[nodiscard]] auto input_state() const -> input::State const&;

	[[nodiscard]] auto is_running() const -> bool { return glfwWindowShouldClose(m_window.get()) != GLFW_TRUE; }
	[[nodiscard]] auto next_frame() -> bool;
	auto render(std::span<Ptr<graphics::RenderPass> const> passes) -> void;
	auto shutdown() -> void;

	Engine([[maybe_unused]] ConstructTag tag) noexcept {}

  private:
	struct Deleter {
		void operator()(GLFWwindow* ptr) const;
	};

	std::unique_ptr<Resources> m_resources{};

	std::unique_ptr<GLFWwindow, Deleter> m_window{};
	std::unique_ptr<graphics::Device> m_graphics_device{};
	std::unique_ptr<graphics::Renderer> m_renderer{};

	std::optional<std::uint32_t> m_image_index{};
};

class Engine::Builder {
  public:
	static constexpr auto default_extent_v{glm::uvec2{1280, 720}};

	explicit Builder(std::string title) : m_title(std::move(title)) {}

	auto set_extent(glm::uvec2 value) -> Builder&;
	auto set_shader_layout(graphics::ShaderLayout shader_layout) -> Builder&;

	[[nodiscard]] auto build() -> std::unique_ptr<Engine>;

  private:
	std::optional<graphics::ShaderLayout> m_shader_layout{};
	std::string m_title{};
	glm::uvec2 m_extent{default_extent_v};
};
} // namespace spaced
