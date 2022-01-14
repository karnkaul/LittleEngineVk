#pragma once
#include <core/std_types.hpp>
#include <glm/mat4x4.hpp>
#include <levk/graphics/basis.hpp>

namespace le::graphics {
// Camera faces -Z
struct Camera {
	struct Z {
		f32 near;
		f32 far;
	};

	static constexpr f32 defaultFOV = 45.0f;
	static constexpr Z default3Dz = {0.1f, 10000.0f};
	static constexpr Z default2Dz = {-100.0f, 100.0f};

	glm::vec3 position{};
	glm::quat orientation{identity};
	f32 fov{defaultFOV};

	static constexpr f32 safe(f32 in, f32 fallback = 1.0f) noexcept { return in <= 0.0f ? fallback : in; }
	static constexpr Z safe(Z in, Z fallback) noexcept { return in.far - in.near <= 0.0f ? fallback : in; }

	Camera& look(glm::vec3 const& target, glm::vec3 const& nUp = up) noexcept { return face(nvec3(target - position), nUp); }
	Camera& face(nvec3 const& direction, glm::vec3 const& nUp = up) noexcept;
	glm::mat4 view() const noexcept { return glm::toMat4(glm::conjugate(orientation)) * glm::translate(glm::mat4(1.0f), -position); }
	glm::mat4 perspective(f32 aspect, Z nf = default3Dz) const noexcept;
	glm::mat4 perspective(glm::vec2 size, Z nf = default3Dz) const noexcept;
	glm::mat4 ortho(glm::vec2 size, Z nf = default2Dz) const noexcept;
};

// impl

inline Camera& Camera::face(nvec3 const& direction, glm::vec3 const& nUp) noexcept {
	orientation = glm::quatLookAt(direction, nUp);
	return *this;
}
inline glm::mat4 Camera::perspective(f32 aspect, Z nf) const noexcept {
	nf = safe(nf, default3Dz);
	return glm::perspective(glm::radians(safe(fov, defaultFOV)), safe(aspect), nf.near, nf.far);
}
inline glm::mat4 Camera::perspective(glm::vec2 size, Z nf) const noexcept {
	nf = safe(nf, default3Dz);
	return glm::perspectiveFov(safe(fov, defaultFOV), safe(size.x), safe(size.y), nf.near, nf.far);
}
inline glm::mat4 Camera::ortho(glm::vec2 xy, Z nf) const noexcept {
	nf = safe(nf, default2Dz);
	xy = {safe(xy.x * 0.5f), safe(xy.y * 0.5f)};
	return glm::ortho(-xy.x, xy.x, -xy.y, xy.y, nf.near, nf.far);
}
} // namespace le::graphics
