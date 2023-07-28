#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <spaced/core/radians.hpp>
#include <spaced/scene/component.hpp>

namespace spaced {
class FreecamController : public Component {
  public:
	auto tick(Duration dt) -> void override;

	glm::vec3 move_speed{10.0f};
	float look_speed{0.3f};
	Radians pitch{};
	Radians yaw{};

  protected:
	glm::vec2 m_prev_cursor{};
};
} // namespace spaced
