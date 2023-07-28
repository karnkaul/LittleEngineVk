#include <spaced/scene/scene_renderer.hpp>

namespace spaced {
SceneRenderer::SceneRenderer(NotNull<Scene const*> scene, graphics::Rgba const clear) : scene(scene) {
	auto const rgba = graphics::Rgba::to_linear(clear.to_tint());
	clear_colour = std::array{rgba.x, rgba.y, rgba.z, rgba.w};
}

auto SceneRenderer::render(vk::CommandBuffer const cmd) -> void {
	auto render_list = std::vector<graphics::RenderObject>{};
	scene->render_to(render_list);
	auto const render_camera = graphics::RenderCamera{
		.camera = &scene->camera,
		.lights = &scene->lights,
	};
	render_objects(render_camera, render_list, cmd);
}
} // namespace spaced
