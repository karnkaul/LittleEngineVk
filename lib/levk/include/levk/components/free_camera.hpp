#pragma once
#include <levk/scene.hpp>

namespace levk {
class FreeCamera : public ISceneCamera {
  public:
	[[nodiscard]] auto get_im_label() const -> CString final { return "FreeCamera"; }

	int control_mouse_button{GLFW_MOUSE_BUTTON_2};

	levk::Degrees pitch{};
	levk::Degrees yaw{};

	float move_speed{10.0f};
	float look_speed{20.0f};
	float look_lerp{0.8f};

  protected:
	void tick(Entity& entity, Seconds dt) override;
	virtual auto control(input::State const& input_state, Seconds dt) -> glm::vec3;

	void im_inspect() override;

	std::optional<glm::vec2> m_prev_cursor{};
	bool m_control{};
};
} // namespace levk
