#pragma once
#include <core/utils/dirty_flag.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace le {
struct Matrix {
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	static Matrix const identity;

	glm::mat4 mat4() const noexcept;
	bool isotropic() const noexcept { return scale.x == scale.y && scale.y == scale.z; }

	static glm::vec3 worldPosition(glm::mat4 const& mat) noexcept { return mat[3]; }
	static glm::quat worldOrientation(glm::mat4 const& mat) noexcept;
	static glm::vec3 worldScale(glm::mat4 const& mat) noexcept;
};

class Transform : utils::DirtyFlag {
  public:
	Transform& position(glm::vec3 const& position) noexcept { return (m_matrix.position = position, makeDirty()); }
	Transform& orient(glm::quat const& orientation) noexcept { return (m_matrix.orientation = orientation, makeDirty()); }
	Transform& rotate(f32 radians, glm::vec3 const& axis) noexcept { return orient(glm::rotate(m_matrix.orientation, radians, axis)); }
	Transform& scale(f32 scale) noexcept { return this->scale({scale, scale, scale}); }
	Transform& scale(glm::vec3 const& scale) noexcept { return (m_matrix.scale = scale, makeDirty()); }
	Transform& reset(Matrix const& matrix = {}) { return (m_matrix = matrix, makeDirty()); }

	glm::vec3 const& position() const noexcept { return m_matrix.position; }
	glm::quat const& orientation() const noexcept { return m_matrix.orientation; }
	glm::vec3 const& scale() const noexcept { return m_matrix.scale; }
	Matrix const& data() const noexcept { return m_matrix; }
	bool isotropic() const noexcept { return m_matrix.isotropic(); }

	glm::mat4 matrix() const noexcept { return (refresh(), m_mat); }

	bool stale() const noexcept { return dirty(); }
	void refresh() const noexcept {
		if (stale()) { m_mat = m_matrix.mat4(); }
	}

  private:
	Transform& makeDirty() noexcept { return (setDirty(), *this); }

	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	Matrix m_matrix;
};

// impl

inline glm::mat4 Matrix::mat4() const noexcept {
	static constexpr auto base = glm::mat4(1.0f);
	auto const t = glm::translate(base, position);
	auto const r = glm::toMat4(orientation);
	auto const s = glm::scale(base, scale);
	return t * r * s;
}
} // namespace le
