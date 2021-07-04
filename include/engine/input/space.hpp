#pragma once
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
		glm::vec2 area{};
		f32 scale = 1.0f;
	} render;
	struct {
		glm::vec2 size{};
		glm::vec2 density = {1.0f, 1.0f};
	} scene;
	struct {
		glm::vec2 offset{};
		f32 scale = 1.0f;
	} viewport;

	static Space make(glm::vec2 scene, glm::uvec2 swap, glm::uvec2 win, Viewport const& view, f32 rscale) noexcept;

	glm::vec2 unproject(glm::vec2 screen, bool normalised) const noexcept;
	glm::vec2 project(glm::vec2 ui, bool normalised) const noexcept;
};
} // namespace le::input
