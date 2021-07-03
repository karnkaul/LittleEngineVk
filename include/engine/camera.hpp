#pragma once
#include <core/std_types.hpp>
#include <glm/mat4x4.hpp>
#include <graphics/basis.hpp>

namespace le {
// Camera faces -Z
struct Camera {
	struct ZPlane {
		f32 near;
		f32 far;
	};

	static constexpr f32 defaultFOV = 45.0f;
	static constexpr ZPlane default3Dz = {0.1f, 100.0f};
	static constexpr ZPlane default2Dz = {-1.0f, 1.0f};

	glm::vec3 position{};
	glm::quat orientation{graphics::identity};
	f32 fov{defaultFOV};

	static f32 safe(f32 in, f32 fallback = 1.0f) noexcept;
	static ZPlane safe(ZPlane in, ZPlane fallback) noexcept;

	Camera& look(glm::vec3 const& target, glm::vec3 const& up = graphics::up) noexcept;
	Camera& face(glm::vec3 const& direction, glm::vec3 const& up = graphics::up) noexcept;
	glm::mat4 view() const noexcept;
	glm::mat4 perspective(f32 aspect, ZPlane nf = default3Dz) const noexcept;
	glm::mat4 perspective(glm::vec2 size, ZPlane nf = default3Dz) const noexcept;
	glm::mat4 ortho(glm::vec2 size, ZPlane nf = default2Dz) const noexcept;
};

// impl

inline f32 Camera::safe(f32 in, f32 fallback) noexcept { return in <= 0.0f ? fallback : in; }
inline Camera::ZPlane Camera::safe(ZPlane in, ZPlane fallback) noexcept { return in.far - in.near <= 0.0f ? fallback : in; }
inline Camera& Camera::look(glm::vec3 const& target, glm::vec3 const& up) noexcept { return face(target - position, up); }
inline Camera& Camera::face(glm::vec3 const& direction, glm::vec3 const& up) noexcept {
	orientation = glm::quatLookAt(glm::normalize(direction), up);
	return *this;
}
inline glm::mat4 Camera::view() const noexcept { return glm::toMat4(glm::conjugate(orientation)) * glm::translate(glm::mat4(1.0f), -position); }
inline glm::mat4 Camera::perspective(f32 aspect, ZPlane nf) const noexcept {
	nf = safe(nf, default3Dz);
	return glm::perspective(glm::radians(safe(fov, defaultFOV)), safe(aspect), nf.near, nf.far);
}
inline glm::mat4 Camera::perspective(glm::vec2 size, ZPlane nf) const noexcept {
	nf = safe(nf, default3Dz);
	return glm::perspectiveFov(safe(fov, defaultFOV), safe(size.x), safe(size.y), nf.near, nf.far);
}
inline glm::mat4 Camera::ortho(glm::vec2 xy, ZPlane nf) const noexcept {
	nf = safe(nf, default2Dz);
	xy = {safe(xy.x * 0.5f), safe(xy.y * 0.5f)};
	return glm::ortho(-xy.x, xy.x, -xy.y, xy.y, nf.near, nf.far);
}
} // namespace le
