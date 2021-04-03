#pragma once
#include <algorithm>
#include <core/std_types.hpp>
#include <glm/vec2.hpp>
#include <graphics/screen_rect.hpp>

namespace le {
struct Viewport final {
	using ScreenRect = graphics::ScreenRect;

	template <typename T>
	struct Axis {
		T n = {};
		T offset = {};
	};

	Axis<glm::vec2> topLeft;
	f32 scale = 1.0f;

	constexpr Viewport() = default;
	constexpr Viewport(glm::vec2 topLeft) noexcept;
	constexpr Viewport(glm::vec2 topLeft, f32 scale) noexcept;
	constexpr Viewport(glm::vec2 topLeft, glm::vec2 offset, f32 scale) noexcept;

	constexpr ScreenRect rect() const noexcept;
	constexpr glm::vec2 size() const noexcept;
	constexpr glm::vec2 centre() const noexcept;

	constexpr Viewport& operator*=(Viewport const& rhs) noexcept;
};

constexpr Viewport operator*(Viewport const& lhs, Viewport const& rhs) noexcept {
	return Viewport(lhs) *= rhs;
}
// impl

constexpr Viewport::Viewport(glm::vec2 topLeft) noexcept : Viewport(topLeft, {0.0f, 0.0f}, 1.0f) {
}
constexpr Viewport::Viewport(glm::vec2 topLeft, f32 scale) noexcept : Viewport(topLeft, {0.0f, 0.0f}, scale) {
}
constexpr Viewport::Viewport(glm::vec2 topLeft, glm::vec2 offset, f32 scale) noexcept : topLeft{topLeft, offset}, scale(scale) {
}

constexpr Viewport::ScreenRect Viewport::rect() const noexcept {
	return ScreenRect::sizeTL({scale, scale}, topLeft.n);
}

constexpr glm::vec2 Viewport::size() const noexcept {
	return glm::vec2(scale, scale);
}

constexpr glm::vec2 Viewport::centre() const noexcept {
	return topLeft.n + (size() * 0.5f);
}

constexpr Viewport& Viewport::operator*=(Viewport const& by) noexcept {
	scale *= by.scale;
	topLeft.n += by.topLeft.n;
	topLeft.offset += by.topLeft.offset;
	return *this;
}
} // namespace le
