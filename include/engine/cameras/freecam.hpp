#pragma once
#include <core/time.hpp>
#include <engine/camera.hpp>
#include <engine/input/control.hpp>

namespace le {
namespace window {
class DesktopInstance;
}

class FreeCam : public Camera {
  public:
	using Desktop = window::DesktopInstance;

	void tick(Input::State const& state, Time_s dt, Desktop* desktop = nullptr);

	struct {
		f32 xz_speed = 1.0f;
		glm::vec2 xz_speed_limit = {0.01f, 100.0f};
		f32 look_sens = 1.0f;
	} m_params;

	struct {
		Control::Range mov_x = Control::KeyRange{Input::Key::eA, Input::Key::eD};
		Control::Range mov_z = Control::KeyRange{Input::Key::eS, Input::Key::eW};
		Control::Range speed = Control::AxisRange{0, Input::Axis::eMouseScrollY};

		Control::Trigger look = {Input::Key::eMouseButton2, Input::Action::eHeld};
		Control::Trigger look_toggle = {Input::Key::eL, Input::Action::ePressed, Input::Mod::eControl};
	} m_controls;

  private:
	struct {
		std::optional<glm::vec2> prev;
	} m_cursor;
	bool m_look = false;
};
} // namespace le