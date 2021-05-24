#pragma once
#include <algorithm>
#include <core/std_types.hpp>
#include <engine/render/flex.hpp>
#include <glm/vec2.hpp>
#include <graphics/screen_rect.hpp>

namespace le {
struct Viewport final {
	using ScreenRect = graphics::ScreenRect;

	TFlex<glm::vec2> topLeft;
	f32 scale = 1.0f;

	constexpr Viewport() = default;
	constexpr Viewport(glm::vec2 topLeft) noexcept;
	constexpr Viewport(glm::vec2 topLeft, f32 scale) noexcept;
	constexpr Viewport(glm::vec2 topLeft, glm::vec2 offset, f32 scale) noexcept;

	constexpr ScreenRect rect() const noexcept;
	constexpr glm::vec2 size() const noexcept;
	constexpr glm::vec2 centre(glm::vec2 coeff = {1.0f, 1.0f}) const noexcept;

	constexpr Viewport& operator*=(Viewport const& rhs) noexcept;
};

constexpr Viewport operator*(Viewport const& lhs, Viewport const& rhs) noexcept { return Viewport(lhs) *= rhs; }
// impl

constexpr Viewport::Viewport(glm::vec2 topLeft) noexcept : Viewport(topLeft, {0.0f, 0.0f}, 1.0f) {}
constexpr Viewport::Viewport(glm::vec2 topLeft, f32 scale) noexcept : Viewport(topLeft, {0.0f, 0.0f}, scale) {}
constexpr Viewport::Viewport(glm::vec2 topLeft, glm::vec2 offset, f32 scale) noexcept : topLeft{topLeft, offset}, scale(scale) {}

constexpr Viewport::ScreenRect Viewport::rect() const noexcept { return ScreenRect::sizeTL({scale, scale}, topLeft.norm); }

constexpr glm::vec2 Viewport::size() const noexcept { return glm::vec2(scale, scale); }

constexpr glm::vec2 Viewport::centre(glm::vec2 coeff) const noexcept { return (coeff * topLeft.norm + (size() * 0.5f)) + topLeft.offset; }

constexpr Viewport& Viewport::operator*=(Viewport const& by) noexcept {
	scale *= by.scale;
	topLeft += by.topLeft;
	return *this;
}
} // namespace le
