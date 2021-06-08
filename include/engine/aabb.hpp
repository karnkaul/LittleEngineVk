#pragma once
#include <core/std_types.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace le {
struct AABB {
	glm::vec2 origin = {};
	glm::vec2 size = {};

	constexpr glm::vec3 position(f32 zIndex = 0.0f) const noexcept { return {origin, zIndex}; }
	constexpr bool hit(glm::vec2 point) const noexcept { return hit(origin, halfSize(), point); }
	constexpr glm::vec2 halfSize() const noexcept { return {size.x * 0.5f, -size.y * 0.5f}; }

	static constexpr bool hit(glm::vec2 centre, glm::vec2 half, glm::vec2 point) noexcept { return in(point, centre - half, centre + half); }
	static constexpr bool in(glm::vec2 pt, glm::vec2 tl, glm::vec2 br) noexcept { return pt.x >= tl.x && pt.x <= br.x && pt.y >= br.y && pt.y <= tl.y; }
};
} // namespace le
