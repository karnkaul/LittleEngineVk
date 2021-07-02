#pragma once
#include <optional>
#include <core/maths.hpp>
#include <core/std_types.hpp>
#include <engine/render/viewport.hpp>
#include <glm/vec2.hpp>

namespace le::input {
struct Space {
	struct {
		glm::vec2 window{};
		glm::vec2 swapchain{};
		glm::vec2 density = {1.0f, 1.0f};
	} display;
	struct {
		glm::vec2 offset{};
		glm::vec2 density{};
		f32 scale = 1.0f;
	} viewport;
	struct {
		glm::vec2 area{};
		f32 scale = 1.0f;
	} render;

	template <typename... T>
	static constexpr bool zero(T const&... t) noexcept;
	static constexpr Space make(glm::uvec2 swap, glm::uvec2 win, Viewport const& view, f32 rscale) noexcept;

	constexpr glm::vec2 unproject(glm::vec2 screen, bool normalised) const noexcept;
	constexpr glm::vec2 project(glm::vec2 world, bool normalised) const noexcept;
};

// impl

template <typename... T>
constexpr bool Space::zero(T const&... t) noexcept {
	return ((maths::equals(t.x, 0.0f) || maths::equals(t.y, 0.0f)) || ...);
}
constexpr Space Space::make(glm::uvec2 swap, glm::uvec2 win, const Viewport& view, f32 renderScale) noexcept {
	Space ret;
	ret.display.swapchain = swap;
	ret.render.scale = renderScale;
	ret.render.area = ret.display.swapchain * ret.render.scale;
	if (!zero(ret.display.window = win)) { ret.display.density = ret.display.swapchain / ret.display.window; }
	if ((ret.viewport.scale = view.scale) != 1.0f && !zero(ret.display.window)) {
		ret.viewport.density = ret.display.window / ret.render.area;
		ret.viewport.offset = ret.display.window * view.topLeft.norm + view.topLeft.offset * ret.viewport.density;
	}
	return ret;
}
constexpr glm::vec2 Space::unproject(glm::vec2 screen, bool normalised) const noexcept {
	if (normalised) { screen *= display.window; }
	if (viewport.scale < 1.0f) { screen = (screen - viewport.offset) / viewport.scale; }
	screen *= display.density;
	screen -= (display.swapchain * 0.5f);
	return {screen.x, -screen.y};
}
constexpr glm::vec2 Space::project(glm::vec2 world, bool normalised) const noexcept {
	if (normalised) { world *= display.swapchain; }
	world = {world.x, -world.y};
	world += (display.swapchain * 0.5f);
	world /= display.density;
	if (viewport.scale < 1.0f) { world = (world * viewport.scale) + viewport.offset; }
	return world;
}
} // namespace le::input
