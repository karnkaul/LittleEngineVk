#pragma once
#include <GLFW/glfw3.h>
#include <levk/core/c_string.hpp>
#include <levk/core/is_positive.hpp>
#include <levk/core/not_null.hpp>
#include <levk/input/event_sink.hpp>
#include <levk/input/state.hpp>
#include <levk/render_device.hpp>

namespace levk {
struct EngineCreateInfo {
	glm::ivec2 window_size{1280, 720};
	CString window_title{"levk"};
	bool show_window_immediately{true};
	CString app_name{"App"};
	Version app_version{};
	Ptr<IGpuSelector> gpu_selector{};
};

/// \brief Abstract class representing the Engine instance.
class IEngine : public Polymorphic {
  public:
	[[nodiscard]] virtual auto get_cursor_position() const -> glm::ivec2 = 0;
	[[nodiscard]] virtual auto get_window_size() const -> glm::ivec2 = 0;
	[[nodiscard]] virtual auto get_framebuffer_size() const -> glm::ivec2 = 0;

	[[nodiscard]] auto get_framebuffer_aspect_ratio() const -> float {
		glm::vec2 const size = get_framebuffer_size();
		if (!is_positive(size)) { return 0.0f; }
		return size.x / size.y;
	}

	[[nodiscard]] virtual auto get_window() const -> Ptr<GLFWwindow> = 0;
	[[nodiscard]] virtual auto is_window_open() const -> bool = 0;

	virtual void toggle_window(bool show) = 0;
	virtual void shutdown() = 0;

	virtual void poll_events(IEventSink& event_sink) = 0;

	[[nodiscard]] virtual auto get_render_device() const -> IRenderDevice& = 0;
	[[nodiscard]] virtual auto get_input_state() const -> input::State const& = 0;
};

/// \brief Create a concrete Engine.
auto create_engine(EngineCreateInfo const& create_info = {}) noexcept(false) -> std::unique_ptr<IEngine>;
} // namespace levk
