#include <spaced/scene/ui/primitive_renderer.hpp>

namespace spaced::ui {
auto PrimitiveRenderer::set_quad(graphics::Quad quad) -> void { primitive.set_geometry(graphics::Geometry::from(quad)); }

auto PrimitiveRenderer::render_tree(std::vector<graphics::RenderObject>& out) const -> void {
	render_to(out, &material, &primitive);
	Renderable::render_tree(out);
}

auto Quad::tick(Duration dt) -> void {
	PrimitiveRenderer::tick(dt);

	set_quad(graphics::Quad{.size = transform.extent});
}
} // namespace spaced::ui
