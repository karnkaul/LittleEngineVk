#include <spaced/engine/camera.hpp>
#include <spaced/engine/core/visitor.hpp>

namespace spaced {
namespace {
auto ortho_extent(glm::vec2 const extent, glm::vec2 const fixed_view) -> glm::vec2 {
	if (fixed_view.x > 0.0f && fixed_view.y > 0.0f) { return fixed_view; }
	return extent;
}
} // namespace

auto Camera::view() const -> glm::mat4 {
	auto const target = transform.position() + transform.orientation() * (face == Face::ePositiveZ ? front_v : -front_v);
	return glm::lookAt(transform.position(), target, up_v);
}

auto Camera::projection(glm::vec2 extent) const -> glm::mat4 {
	if (extent.x <= 0.0f || extent.y <= 0.0f) { return glm::identity<glm::mat4>(); }
	auto const visitor = Visitor{
		[extent](Perspective const& p) { return glm::perspective(p.field_of_view.value, extent.x / extent.y, p.view_plane.near, p.view_plane.far); },
		[extent](Orthographic const& o) { return orthographic(ortho_extent(extent, o.fixed_view), o.view_plane); },
	};
	return std::visit(visitor, type);
}

auto Camera::orthographic(glm::vec2 const extent, ViewPlane const view_plane) -> glm::mat4 {
	auto const he = 0.5f * extent;
	return glm::ortho(-he.x, he.x, -he.y, he.y, view_plane.near, view_plane.far);
}
} // namespace spaced
