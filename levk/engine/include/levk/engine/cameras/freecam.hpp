#pragma once
#include <levk/core/time.hpp>
#include <levk/engine/input/control.hpp>
#include <levk/graphics/render/camera.hpp>

namespace le {
namespace window {
class Window;
}

class FreeCam : public graphics::Camera {
  public:
	using Window = window::Window;

	void tick(input::State const& state, Time_s dt, Window* win = nullptr);

	struct {
		f32 xz_speed = 1.0f;
		glm::vec2 xz_speed_limit = {0.01f, 100.0f};
		f32 look_sens = 1.0f;
	} m_params;

	struct {
		input::Range mov_x = input::KeyRange{input::Key::eA, input::Key::eD};
		input::Range mov_z = input::KeyRange{input::Key::eS, input::Key::eW};
		input::Range speed = input::AxisRange{input::Axis::eMouseScrollY};

		input::Trigger look = input::Hold{input::Key::eMouseButton2};
		input::Trigger look_toggle = {input::Key::eL, input::Action::ePress, input::Mod::eCtrl};
	} m_controls;

  private:
	struct {
		std::optional<glm::vec2> prev;
	} m_cursor;
	bool m_look = false;
};
} // namespace le
