#pragma once
#include <algorithm>
#include <core/std_types.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <glm/vec2.hpp>

namespace le::gfx {
struct Viewport final {
	glm::vec2 topLeft = {0.0f, 0.0f};
	f32 scale = 1.0f;

	constexpr ScreenRect rect() const noexcept;
	constexpr glm::vec2 size() const noexcept;
	constexpr glm::vec2 centre() const noexcept;

	f32 clampScale(f32 s, glm::vec2 limit = {0.0f, 1.0f}) noexcept;
	glm::vec2 clampTopLeft(glm::vec2 tl, glm::vec2 limit = {0.0f, 1.0f}) noexcept;
	f32 clampTop(f32 top, glm::vec2 limit = {0.0f, 1.0f}) noexcept;
	f32 clampLeft(f32 left, glm::vec2 limit = {0.0f, 1.0f}) noexcept;
};

// impl

inline constexpr ScreenRect Viewport::rect() const noexcept {
	return ScreenRect::sizeTL({scale, scale}, topLeft);
}

inline constexpr glm::vec2 Viewport::size() const noexcept {
	return glm::vec2(scale, scale);
}

inline constexpr glm::vec2 Viewport::centre() const noexcept {
	return topLeft + (size() * 0.5f);
}

inline f32 Viewport::clampScale(f32 s, glm::vec2 limit) noexcept {
	return scale = std::clamp(s, limit.x, limit.y);
}

inline glm::vec2 Viewport::clampTopLeft(glm::vec2 tl, glm::vec2 limit) noexcept {
	return topLeft = {std::clamp(tl.x, limit.x, limit.y), std::clamp(tl.y, limit.x, limit.y)};
}

inline f32 Viewport::clampTop(f32 top, glm::vec2 limit) noexcept {
	return topLeft.y = std::clamp(top, limit.x, limit.y);
}
inline f32 Viewport::clampLeft(f32 left, glm::vec2 limit) noexcept {
	return topLeft.x = std::clamp(left, limit.x, limit.y);
}
} // namespace le::gfx
