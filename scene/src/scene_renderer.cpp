#include <le/scene/scene_renderer.hpp>

namespace le {
SceneRenderer::SceneRenderer() {
	m_skybox_cube.set_geometry(graphics::Geometry::from(graphics::Cube{.size = glm::vec3{1.0f}}));
	m_skybox_pipeline.depth_test_write = 0;
}

auto SceneRenderer::render(Scene const& scene) -> graphics::RenderFrame {
	m_scene_objects.clear();
	m_ui_objects.clear();

	if (scene.skybox != nullptr) {
		m_skybox_mat.texture = scene.skybox;
		m_scene_objects.push_back(graphics::RenderObject{
			.material = &m_skybox_mat,
			.primitive = &m_skybox_cube,
			.pipeline_state = m_skybox_pipeline,
		});
	}
	scene.render_entities(m_scene_objects);
	scene.get_ui_root().render_tree(m_ui_objects);

	return graphics::RenderFrame{
		.lights = &scene.lights,
		.camera = &scene.main_camera,
		.scene = m_scene_objects,
		.ui = m_ui_objects,
	};
}
} // namespace le
