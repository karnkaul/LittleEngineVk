#include <le/scene/ui/primitive_renderer.hpp>

namespace le::ui {
auto PrimitiveRenderer::set_quad(graphics::Quad quad) -> void { primitive.set_geometry(graphics::Geometry::from(quad)); }

auto PrimitiveRenderer::render(std::vector<graphics::RenderObject>& out) const -> void {
	if (!is_active()) { return; }

	render_to(out, &material, &primitive);
}
} // namespace le::ui
