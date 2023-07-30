#include <spaced/scene/ui/primitive_renderer.hpp>

namespace spaced::ui {
auto PrimitiveRenderer::set_quad(graphics::Quad quad) -> void { primitive.set_geometry(graphics::Geometry::from(quad)); }

auto PrimitiveRenderer::render_tree(Rect2D<> const& parent_frame, std::vector<graphics::RenderObject>& out) const -> void {
	auto const obj = graphics::RenderObject{
		.material = &material,
		.primitive = &primitive,
		.parent = transform.matrix(),
		.instances = instances,
		.pipeline_state = pipeline_state,
	};
	out.push_back(obj);

	View::render_tree(parent_frame, out);
}
} // namespace spaced::ui
