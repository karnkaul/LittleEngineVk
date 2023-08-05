#pragma once
#include <le/graphics/rect.hpp>

namespace le::graphics {
struct Glyph {
	glm::vec2 advance{};
	glm::vec2 extent{};
	glm::vec2 left_top{};
	UvRect uv_rect{};

	[[nodiscard]] constexpr auto rect(glm::vec2 baseline, float const scale = 1.0f) const -> Rect2D<> {
		return {.lt = baseline + scale * left_top, .rb = baseline + scale * (left_top + glm::vec2{extent.x, -extent.y})};
	}

	explicit constexpr operator bool() const { return advance.x > 0 || advance.y > 0; }
};
} // namespace le::graphics
