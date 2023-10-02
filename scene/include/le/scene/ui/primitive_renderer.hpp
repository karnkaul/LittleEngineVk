#pragma once
#include <le/scene/ui/renderable.hpp>

namespace le::ui {
class PrimitiveRenderer : public Renderable {
  public:
	using Renderable::Renderable;

	auto set_quad(graphics::Quad quad) -> void;

	auto tick(Duration /*dt*/) -> void override {}
	auto render(std::vector<graphics::RenderObject>& out) const -> void override;

	graphics::DynamicPrimitive primitive{};
	graphics::UnlitMaterial material{};
};
} // namespace le::ui
