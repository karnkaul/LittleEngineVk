#pragma once
#include <optional>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <glm/vec3.hpp>

namespace le {

struct SpringArm_OLD {
	glm::vec3 offset = {};
	glm::vec3 position = {};
	Transform const* pTarget = nullptr;
	Time_s fixed = 2ms;
	f32 k = 0.5f;
	f32 b = 0.05f;
	struct {
		glm::vec3 velocity = {};
		Time_s ft;
	} data;

	glm::vec3 const& tick(Time_s dt, std::optional<glm::vec3> target = std::nullopt) noexcept;
};
} // namespace le
