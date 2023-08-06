#include <le/scene/scene_renderer.hpp>

namespace le {
SceneRenderer::SceneRenderer() {
	m_skybox_cube.set_geometry(graphics::Geometry::from(graphics::Cube{.size = glm::vec3{1.0f}}));
	m_skybox_pipeline.depth_test_write = 0;
}

auto SceneRenderer::render(Scene const& scene) -> graphics::RenderFrame const& {
	if (!m_render_frame) {
		m_render_frame.emplace(graphics::RenderFrame{
			.lights = &scene.lights,
			.camera = &scene.main_camera,
			.clear_colour = scene.main_camera.clear_colour,
		});
	}

	m_render_frame->scene.clear();
	m_render_frame->ui.clear();

	m_render_frame->lights = &scene.lights;
	m_render_frame->camera = &scene.main_camera;
	m_render_frame->clear_colour = scene.main_camera.clear_colour;

	if (scene.skybox != nullptr) {
		m_skybox_mat.texture = scene.skybox;
		m_render_frame->scene.push_back(graphics::RenderObject{
			.material = &m_skybox_mat,
			.primitive = &m_skybox_cube,
			.pipeline_state = m_skybox_pipeline,
		});
	}
	scene.render_entities(m_render_frame->scene);
	scene.get_ui_root().render_tree(m_render_frame->ui);

	return *m_render_frame;
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
