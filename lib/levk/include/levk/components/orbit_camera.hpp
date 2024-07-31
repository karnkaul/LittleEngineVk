#pragma once
#include <levk/scene.hpp>

namespace levk {
class OrbitCamera : public ISceneCamera {
  public:
	[[nodiscard]] auto get_im_label() const -> CString final { return "OrbitCamera"; }

	int control_mouse_button{GLFW_MOUSE_BUTTON_1};

	levk::Degrees pitch{};
	levk::Degrees yaw{};

	glm::vec3 origin{};
	float radius{5.0f};
	levk::Degrees orbit_rate{100.0f};
	float zoom_speed{1.0f};

  protected:
	void tick(Entity& entity, Seconds dt) override;
	virtual void control(input::State const& input_state, Seconds dt);

	void im_inspect() override;

	std::optional<glm::vec2> m_prev_cursor{};
	bool m_control{};
};
} // namespace levk
