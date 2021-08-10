#pragma once
#include <core/time.hpp>
#include <engine/input/control.hpp>
#include <graphics/render/camera.hpp>

namespace le {
namespace window {
class Instance;
}

class FreeCam : public graphics::Camera {
  public:
	using Window = window::Instance;

	void tick(input::State const& state, Time_s dt, Window* win = nullptr);

	struct {
		f32 xz_speed = 1.0f;
		glm::vec2 xz_speed_limit = {0.01f, 100.0f};
		f32 look_sens = 1.0f;
	} m_params;

	struct {
		input::Range mov_x = input::KeyRange{input::Key::eA, input::Key::eD};
		input::Range mov_z = input::KeyRange{input::Key::eS, input::Key::eW};
		input::Range speed = input::AxisRange{0, input::Axis::eMouseScrollY};

		input::Trigger look = {input::Key::eMouseButton2, input::Action::eHeld};
		input::Trigger look_toggle = {input::Key::eL, input::Action::ePressed, input::Mod::eControl};
	} m_controls;

  private:
	struct {
		std::optional<glm::vec2> prev;
	} m_cursor;
	bool m_look = false;
};
} // namespace le
