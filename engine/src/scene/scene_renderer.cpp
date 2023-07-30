#include <spaced/scene/scene_renderer.hpp>

namespace spaced {
SceneRenderer::SceneRenderer(NotNull<Scene const*> scene) : scene(scene) {}

auto SceneRenderer::get_load() const -> Load {
	auto const clear_colour = graphics::Rgba::to_linear(scene->main_camera.clear_colour.to_tint());
	return Load{
		.clear_colour = std::array{clear_colour.x, clear_colour.y, clear_colour.z, clear_colour.w},
		.load_op = vk::AttachmentLoadOp::eClear,
	};
}

auto SceneRenderer::render(vk::CommandBuffer const cmd) -> void {
	m_objects.clear();

	// render 3D
	scene->render_entities(m_objects);
	auto render_camera = graphics::RenderCamera{.camera = &scene->main_camera, .lights = &scene->lights};
	render_objects(render_camera, m_objects, cmd);

	m_objects.clear();

	// render UI
	scene->get_ui_root().render_tree(Rect2D<>::from_extent(framebuffer_extent()), m_objects);
	render_camera.camera = &scene->ui_camera;
	render_objects(render_camera, m_objects, cmd);
}
} // namespace spaced
