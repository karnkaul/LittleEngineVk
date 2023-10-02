#include <le/scene/ui/quad.hpp>

namespace le::ui {
auto Quad::tick(Duration dt) -> void {
	if (!is_active()) { return; }

	PrimitiveRenderer::tick(dt);

	auto quad = graphics::RoundedQuad{};
	quad.size = transform.extent;
	quad.corner_radius = corner_radius;
	primitive.set_geometry(graphics::Geometry::from(quad));
}
} // namespace le::ui
