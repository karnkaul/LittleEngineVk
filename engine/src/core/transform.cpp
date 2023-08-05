#include <glm/gtx/matrix_decompose.hpp>
#include <le/core/transform.hpp>

namespace le {
auto Transform::look_at(glm::vec3 const& point, glm::vec3 const& eye, glm::quat const& start, nvec3 const& up) -> glm::quat {
	glm::vec3 const current_look = start * front_v;
	auto const to_point = point - eye;
	auto const dot = glm::dot(current_look, glm::normalize(to_point));
	// NOLINTNEXTLINE
	if (1.0f - dot < 0.001f) { return glm::identity<glm::quat>(); }
	// NOLINTNEXTLINE
	if (1.0f + dot < 0.001f) { return glm::angleAxis(glm::radians(180.0f), up.value()); }
	auto const axis = glm::normalize(glm::cross(current_look, to_point));
	return glm::angleAxis(glm::acos(dot), axis);
}

auto Transform::decompose(glm::mat4 const& mat) -> Transform& {
	auto skew = glm::vec3{};
	auto persp = glm::vec4{};
	glm::decompose(mat, m_data.scale, m_data.orientation, m_data.position, skew, persp);
	m_matrix = mat;
	m_dirty = false;
	return *this;
}

auto Transform::combined(glm::mat4 const& parent) const -> Transform {
	auto ret = Transform{};
	ret.decompose(parent * matrix());
	return ret;
}
} // namespace le
