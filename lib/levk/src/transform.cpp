#include <glm/gtx/matrix_decompose.hpp>
#include <levk/imcpp/im_controls.hpp>
#include <levk/imcpp/im_text.hpp>
#include <levk/transform.hpp>

namespace levk {
auto Transform::set_position(glm::vec3 const& position) -> Transform& {
	m_data.position = position;
	m_dirty = true;
	return *this;
}

auto Transform::set_orientation(glm::quat const& orientation) -> Transform& {
	m_data.orientation = orientation;
	m_dirty = true;
	return *this;
}

auto Transform::set_scale(glm::vec3 const& scale) -> Transform& {
	m_data.scale = scale;
	m_dirty = true;
	return *this;
}

auto Transform::get_matrix() const -> glm::mat4 const& {
	if (m_dirty) { refresh(); }
	return m_matrix;
}

void Transform::from_matrix(glm::mat4 const& matrix) {
	m_matrix = matrix;
	auto skew = glm::vec3{};
	auto persp = glm::vec4{};
	glm::decompose(matrix, m_data.scale, m_data.orientation, m_data.position, skew, persp);
}

void Transform::set_data(Data const& data) {
	m_data = data;
	m_dirty = true;
}

void Transform::refresh() const {
	m_matrix = m_data.get_matrix();
	m_dirty = false;
}

auto Transform::im_inspect_position(glm::vec3& out_position) -> bool { return ImDragFloat{}.fvec("position", out_position); }

auto Transform::im_inspect_orientation(glm::quat& out_orientation) -> bool { return ImDragFloat{}.euler("orientation", out_orientation); }

auto Transform::im_inspect_scale(glm::vec3& out_scale) -> bool { return ImDragFloat{}.fvec("scale", out_scale, glm::vec3{1.0f}, im_unified_scale); }

void Transform::im_inspect() {
	m_dirty |= im_inspect_position(m_data.position);
	m_dirty |= im_inspect_orientation(m_data.orientation);
	m_dirty |= im_inspect_scale(m_data.scale);
	if (ImGui::Button("reset##reset_transform")) { *this = {}; }
}
} // namespace levk
