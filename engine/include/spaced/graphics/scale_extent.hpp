#pragma once
#include <glm/vec2.hpp>
#include <cassert>

namespace spaced::graphics {
struct ScaleExtent {
	glm::vec2 target{};

	[[nodiscard]] constexpr auto aspect_ratio() const -> float {
		assert(target.x > 0.0f && target.y > 0.0f);
		return target.x / target.y;
	}

	[[nodiscard]] constexpr auto fixed_width(float width) const -> glm::vec2 { return {width, width / aspect_ratio()}; }
	[[nodiscard]] constexpr auto fixed_height(float height) const -> glm::vec2 { return {aspect_ratio() * height, height}; }
};
} // namespace spaced::graphics
