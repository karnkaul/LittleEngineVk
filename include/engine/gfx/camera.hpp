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
struct Camera final
{
	glm::vec3 position = {};
	glm::quat orientation = g_qIdentity;
	f32 fov = 45.0f;

	glm::mat4 view() const noexcept;
	glm::mat4 perspective(f32 aspect, f32 near = 0.1f, f32 far = 100.0f) const noexcept;
	glm::mat4 ortho(f32 aspect, f32 zoom = 1.0f, f32 near = 0.1f, f32 far = 100.0f) const noexcept;
	glm::mat4 ui(glm::vec3 const& uiSpace) const noexcept;

	void reset() noexcept;
};
} // namespace le::gfx
