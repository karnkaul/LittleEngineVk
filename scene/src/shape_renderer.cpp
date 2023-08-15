#include <le/scene/scene.hpp>
#include <le/scene/shape_renderer.hpp>

namespace le {
auto ShapeRenderer::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	if (!material) { return; }
	auto const parent = get_scene().get_node_tree().global_transform(get_entity().get_node());
	out.push_back(graphics::RenderObject{
		.material = material.get(),
		.primitive = &primitive,
		.parent = parent,
		.instances = {&m_instance, 1},
		.pipeline_state = pipeline_state,
	});
}
} // namespace le
