#include <levk/transform.hpp>
#include <levk/util.hpp>

auto levk::get_view_matrix(glm::vec3 const& position, Radians const yaw, Radians const pitch) -> glm::mat4 {
	auto const t = glm::translate(identity_mat_v, -position);
	auto const r = glm::rotate(identity_mat_v, -pitch.value, right_v) * glm::rotate(identity_mat_v, -yaw.value, up_v);
	return r * t;
}
