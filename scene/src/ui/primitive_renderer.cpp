#include <le/scene/ui/primitive_renderer.hpp>

namespace le::ui {
void PrimitiveRenderer::set_quad(graphics::Quad quad) { primitive.set_geometry(graphics::Geometry::from(quad)); }

void PrimitiveRenderer::set_texture(Ptr<graphics::Texture const> texture) { material.texture = texture; }

void PrimitiveRenderer::render(std::vector<graphics::RenderObject>& out) const {
	if (!is_active()) { return; }

	render_to(out, &material, &primitive);
}
} // namespace le::ui
