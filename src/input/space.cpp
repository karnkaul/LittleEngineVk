#include <core/maths.hpp>
#include <engine/input/space.hpp>
#include <graphics/basis.hpp>

namespace le ::input {
Space Space::make(glm::vec2 scene, glm::uvec2 swap, glm::uvec2 win, Viewport const& view, f32 rscale) noexcept {
	Space ret;
	ret.scene.size = scene;
	ret.display.swapchain = swap;
	ret.render.scale = rscale;
	if ((ret.display.window = win.x == 0 ? swap : win).x != 0.0f) {
		ret.display.density = ret.display.swapchain / ret.display.window;
		ret.render.area = ret.display.swapchain * rscale;
		if (ret.scene.size.x > 0.0f) { ret.scene.density = ret.scene.size / ret.display.window; }
	}
	if ((ret.viewport.scale = view.scale) != 1.0f) { ret.viewport.offset = ret.display.window * view.topLeft.norm + view.topLeft.offset; }
	return ret;
}

glm::vec2 Space::unproject(glm::vec2 screen, bool normalised) const noexcept {
	if (normalised) { screen *= display.window; }
	if (viewport.scale < 1.0f) { screen = (screen - viewport.offset) / viewport.scale; }
	screen *= (scene.density);
	screen -= (scene.size * 0.5f);
	return graphics::invertY(screen);
}

glm::vec2 Space::project(glm::vec2 ui, bool normalised) const noexcept {
	if (normalised) { ui *= scene.size; }
	ui = graphics::invertY(ui);
	ui += (scene.size * 0.5f);
	ui /= scene.density;
	if (viewport.scale < 1.0f) { ui = (ui * viewport.scale) + viewport.offset; }
	return ui;
}
} // namespace le::input
