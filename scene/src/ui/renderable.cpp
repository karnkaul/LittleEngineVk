#include <le/scene/ui/renderable.hpp>

namespace le::ui {
auto Renderable::render_to(std::vector<RenderObject>& out, NotNull<Material const*> material, NotNull<Primitive const*> primitive) const -> void {
	render_instance.transform.set_position({transform.position, z_index})
		.set_orientation(glm::angleAxis(rotation.value, front_v))
		.set_scale({transform.scale, 1.0f});
	auto const obj = RenderObject{
		.material = material,
		.primitive = primitive,
		.parent = get_parent_matrix(),
		.instances = {&render_instance, 1},
		.pipeline_state = pipeline_state_v,
	};
	out.push_back(obj);
}
} // namespace le::ui
