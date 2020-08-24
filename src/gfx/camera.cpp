#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <core/assert.hpp>
#include <engine/gfx/geometry.hpp>
#include <engine/gfx/camera.hpp>

namespace le::gfx
{
Camera::Camera() = default;
Camera::Camera(Camera&&) = default;
Camera& Camera::operator=(Camera&&) = default;
Camera::~Camera() = default;

bool s_bTEST = false;
glm::mat4 Camera::view() const
{
#if defined(LEVK_DEBUG)
	if (s_bTEST)
	{
		glm::vec3 const nFront = glm::normalize(m_orientation * -g_nFront);
		glm::vec3 const nUp = glm::normalize(m_orientation * g_nUp);
		return glm::lookAt(m_position, m_position + nFront, nUp);
	}
	else
#endif
	{
		return glm::toMat4(glm::conjugate(m_orientation)) * glm::translate(glm::mat4(1.0f), -m_position);
	}
}

glm::mat4 Camera::perspective(f32 aspect, f32 near, f32 far) const
{
	return glm::perspective(glm::radians(m_fov), aspect, near, far);
}

glm::mat4 Camera::ortho(f32 aspect, f32 zoom, f32 near, f32 far) const
{
	ASSERT(zoom > 0.0f, "Invalid zoom!");
	f32 const ar = aspect;
	f32 const w = ar > 1.0f ? 1.0f : 1.0f * ar;
	f32 const h = ar > 1.0f ? 1.0f / ar : 1.0f;
	return glm::ortho(-w / zoom, w / zoom, -h / zoom, h / zoom, near, far);
}

glm::mat4 Camera::ui(glm::vec3 const& uiSpace) const
{
	f32 const w = uiSpace.x * 0.5f;
	f32 const h = uiSpace.y * 0.5f;
	return glm::ortho(-w, w, -h, h, -uiSpace.z, uiSpace.z);
}

void Camera::reset()
{
	m_position = {};
	m_orientation = g_qIdentity;
	m_fov = 45.0f;
}
} // namespace le::gfx
