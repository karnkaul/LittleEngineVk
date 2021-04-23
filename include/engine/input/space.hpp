#pragma once
#include <optional>
#include <core/maths.hpp>
#include <core/std_types.hpp>
#include <engine/render/viewport.hpp>
#include <glm/vec2.hpp>

namespace le::input {
struct Space {
	glm::vec2 fbSize = {};
	glm::vec2 wSize = {};
	glm::vec2 delta = {};
	f32 scale = 1.0f;

	template <typename... T>
	static constexpr bool anyZero(T const&... t) noexcept;

	constexpr Space() = default;
	constexpr Space(glm::vec2 fbSize, glm::vec2 wSize, Viewport const& view) noexcept;

	constexpr glm::vec2 density() const noexcept;
	constexpr glm::vec2 world(glm::vec2 screen, bool normalised) const noexcept;
	constexpr glm::vec2 screen(glm::vec2 world, bool normalised) const noexcept;
};

// impl

template <typename... T>
constexpr bool Space::anyZero(T const&... t) noexcept {
	return ((maths::equals(t.x, 0.0f) || maths::equals(t.y, 0.0f)) || ...);
}
constexpr Space::Space(glm::vec2 fbSize, glm::vec2 wSize, Viewport const& view) noexcept
	: fbSize(fbSize), wSize(anyZero(wSize) ? fbSize : wSize), delta(this->wSize * view.topLeft.norm + view.topLeft.offset), scale(view.scale) {
}
constexpr glm::vec2 Space::density() const noexcept {
	return anyZero(wSize, fbSize) ? glm::vec2(1.0f) : fbSize / wSize;
}
constexpr glm::vec2 Space::world(glm::vec2 screen, bool normalised) const noexcept {
	if (normalised) {
		screen *= wSize;
	}
	if (scale < 1.0f) {
		screen = (screen - delta) / scale;
	}
	screen *= density();
	screen -= (fbSize * 0.5f);
	return {screen.x, -screen.y};
}
constexpr glm::vec2 Space::screen(glm::vec2 world, bool normalised) const noexcept {
	if (normalised) {
		world *= fbSize;
	}
	world = {world.x, -world.y};
	world += (fbSize * 0.5f);
	world /= density();
	if (scale < 1.0f) {
		world = (world * scale) + delta;
	}
	return world;
}
} // namespace le::input
