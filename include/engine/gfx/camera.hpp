#pragma once
#include <core/time.hpp>
#include <engine/gfx/geometry.hpp>

namespace le
{
class Window;
}

namespace le::gfx
{
// Camera faces -Z
class Camera
{
public:
	glm::vec3 m_position = {};
	glm::quat m_orientation = g_qIdentity;
	f32 m_fov = 45.0f;

public:
	Camera();
	Camera(Camera&&);
	Camera& operator=(Camera&&);
	virtual ~Camera();

public:
	glm::mat4 view() const;
	glm::mat4 perspective(f32 aspect, f32 near = 0.1f, f32 far = 100.0f) const;
	glm::mat4 ortho(f32 aspect, f32 zoom = 1.0f, f32 near = 0.1f, f32 far = 100.0f) const;
	glm::mat4 ui(glm::vec3 const& uiSpace) const;
};
} // namespace le::gfx
