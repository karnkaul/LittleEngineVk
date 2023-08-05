#include <le/scene/scene_renderer.hpp>

namespace le {
SceneRenderer::SceneRenderer(NotNull<Scene const*> scene) : scene(scene) {
	m_skybox_cube.set_geometry(graphics::Geometry::from(graphics::Cube{.size = glm::vec3{1.0f}}));
	m_skybox_pipeline.depth_test_write = 0;
}

auto SceneRenderer::get_load() const -> Load {
	auto const clear_colour = graphics::Rgba::to_linear(scene->main_camera.clear_colour.to_tint());
	return Load{
		.clear_colour = std::array{clear_colour.x, clear_colour.y, clear_colour.z, clear_colour.w},
		.load_op = vk::AttachmentLoadOp::eClear,
	};
}

auto SceneRenderer::render(vk::CommandBuffer const cmd) -> void {
	m_objects.clear();

	// render skybox
	if (scene->skybox != nullptr) { render_skybox(*scene->skybox); }

	// render 3D
	scene->render_entities(m_objects);
	auto render_camera = graphics::RenderCamera{.camera = &scene->main_camera, .lights = &scene->lights};
	render_objects(render_camera, m_objects, cmd);

	m_objects.clear();

	// render UI
	scene->get_ui_root().render_tree(m_objects);
	render_camera.camera = &scene->ui_camera;
	render_objects(render_camera, m_objects, cmd);
}

auto SceneRenderer::render_skybox(graphics::Cubemap const& cubemap) -> void {
	m_skybox_mat.texture = &cubemap;
	auto const object = graphics::RenderObject{
		.material = &m_skybox_mat,
		.primitive = &m_skybox_cube,
		.pipeline_state = m_skybox_pipeline,
	};
	m_objects.push_back(object);
}
} // namespace le
