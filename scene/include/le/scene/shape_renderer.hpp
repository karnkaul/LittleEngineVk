#pragma once
#include <le/graphics/primitive.hpp>
#include <le/scene/render_component.hpp>

namespace le {
class ShapeRenderer : public RenderComponent {
  public:
	auto tick(Duration /*dt*/) -> void override {}
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void override;

	template <typename ShapeT>
	auto set_shape(ShapeT const& shape) -> void {
		primitive.set_geometry(graphics::Geometry::from(shape));
	}

	auto set_tint(graphics::Rgba tint) -> void { m_instance.tint = tint; }
	auto get_tint() const -> graphics::Rgba { return m_instance.tint; }

	graphics::DynamicPrimitive primitive{};
	std::unique_ptr<graphics::Material> material{std::make_unique<graphics::UnlitMaterial>()};
	graphics::PipelineState pipeline_state{};

  protected:
	graphics::RenderInstance m_instance{};
};
} // namespace le
