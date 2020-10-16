#include <algorithm>
#include <core/ensure.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/geometry.hpp>
#include <glm/gtx/quaternion.hpp>

namespace le::gfx {
bool s_bTEST = false;
glm::mat4 Camera::view() const noexcept {
#if defined(LEVK_DEBUG)
	if (s_bTEST) {
		glm::vec3 const nFront = glm::normalize(orientation * -g_nFront);
		glm::vec3 const nUp = glm::normalize(orientation * g_nUp);
		return glm::lookAt(position, position + nFront, nUp);
	} else
#endif
	{
		return glm::toMat4(glm::conjugate(orientation)) * glm::translate(glm::mat4(1.0f), -position);
	}
}

glm::mat4 Camera::perspective(f32 aspect, f32 near, f32 far) const noexcept {
	return glm::perspective(glm::radians(fov), aspect, near, far);
}

glm::mat4 Camera::ortho(f32 aspect, f32 zoom, f32 near, f32 far) const noexcept {
	ENSURE(zoom > 0.0f, "Invalid zoom!");
	f32 const ar = aspect;
	f32 const w = ar > 1.0f ? 1.0f : 1.0f * ar;
	f32 const h = ar > 1.0f ? 1.0f / ar : 1.0f;
	return glm::ortho(-w / zoom, w / zoom, -h / zoom, h / zoom, near, far);
}

glm::mat4 Camera::ui(glm::vec3 const& uiSpace) const noexcept {
	f32 const w = uiSpace.x * 0.5f;
	f32 const h = uiSpace.y * 0.5f;
	return glm::ortho(-w, w, -h, h, -uiSpace.z, uiSpace.z);
}

void Camera::reset() noexcept {
	position = {};
	orientation = g_qIdentity;
	fov = 45.0f;
}
} // namespace le::gfx
