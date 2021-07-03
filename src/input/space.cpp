#include <core/maths.hpp>
#include <engine/input/space.hpp>

namespace le ::input {
Space Space::make(glm::vec2 worldSize, glm::uvec2 swap, glm::uvec2 win, Viewport const& view) noexcept {
	Space ret;
	ret.worldSize = worldSize;
	ret.display.swapchain = swap;
	if ((ret.display.window = win.x == 0 ? swap : win).x != 0.0f) { ret.display.density = ret.display.swapchain / ret.display.window; }
	if ((ret.viewport.scale = view.scale) != 1.0f) { ret.viewport.offset = ret.display.window * view.topLeft.norm + view.topLeft.offset; }
	return ret;
}

glm::vec2 Space::unproject(glm::vec2 screen, bool normalised) const noexcept {
	if (normalised) { screen *= display.window; }
	if (viewport.scale < 1.0f) { screen = (screen - viewport.offset) / viewport.scale; }
	screen *= (worldSize / display.window);
	screen -= (worldSize * 0.5f);
	return glm::vec2{screen.x, -screen.y};
}

glm::vec2 Space::project(glm::vec2 world, bool normalised) const noexcept {
	if (normalised) { world *= worldSize; }
	world = {world.x, -world.y};
	world += (worldSize * 0.5f);
	world *= (display.window / worldSize);
	if (viewport.scale < 1.0f) { world = (world * viewport.scale) + viewport.offset; }
	return world;
}
} // namespace le::input
