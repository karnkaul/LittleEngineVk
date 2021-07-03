#include <core/maths.hpp>
#include <engine/input/space.hpp>

namespace le ::input {
Space Space::make(glm::vec2 world, glm::uvec2 swap, glm::uvec2 win, Viewport const& view) noexcept {
	Space ret;
	ret.world.size = world;
	ret.display.swapchain = swap;
	if ((ret.display.window = win.x == 0 ? swap : win).x != 0.0f) {
		ret.display.density = ret.display.swapchain / ret.display.window;
		if (ret.world.size.x > 0.0f) { ret.world.density = ret.world.size / ret.display.window; }
	}
	if ((ret.viewport.scale = view.scale) != 1.0f) { ret.viewport.offset = ret.display.window * view.topLeft.norm + view.topLeft.offset; }
	return ret;
}

glm::vec2 Space::unproject(glm::vec2 screen, bool normalised) const noexcept {
	if (normalised) { screen *= display.window; }
	if (viewport.scale < 1.0f) { screen = (screen - viewport.offset) / viewport.scale; }
	screen *= (world.density);
	screen -= (world.size * 0.5f);
	return glm::vec2{screen.x, -screen.y};
}

glm::vec2 Space::project(glm::vec2 world, bool normalised) const noexcept {
	if (normalised) { world *= this->world.size; }
	world = {world.x, -world.y};
	world += (this->world.size * 0.5f);
	world /= this->world.density;
	if (viewport.scale < 1.0f) { world = (world * viewport.scale) + viewport.offset; }
	return world;
}
} // namespace le::input
