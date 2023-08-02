#pragma once
#include <spaced/graphics/primitive.hpp>
#include <spaced/scene/render_component.hpp>

namespace spaced {
class ShapeRenderer : public RenderComponent {
  public:
	auto tick(Duration /*dt*/) -> void override {}
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void override;

	template <typename ShapeT>
	auto set_shape(ShapeT const& shape) -> void {
		primitive.set_geometry(graphics::Geometry::from(shape));
	}

	graphics::DynamicPrimitive primitive{};
	std::unique_ptr<graphics::Material> material{std::make_unique<graphics::UnlitMaterial>()};
	graphics::PipelineState pipeline_state{};
	graphics::Rgba tint{};

  protected:
	mutable graphics::RenderInstance m_instance{};
};
} // namespace spaced
