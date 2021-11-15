#include <core/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace le {
glm::quat Matrix::worldOrientation(glm::mat4 const& mat) noexcept {
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(mat, scl, orn, pos, skw, psp);
	return glm::conjugate(orn);
}

glm::vec3 Matrix::worldScale(glm::mat4 const& mat) noexcept {
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(mat, scl, orn, pos, skw, psp);
	return scl;
}

Matrix const Matrix::identity = {};
} // namespace le
