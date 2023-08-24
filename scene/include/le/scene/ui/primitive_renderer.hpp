#pragma once
#include <le/scene/ui/renderable.hpp>

namespace le::ui {
class PrimitiveRenderer : public Renderable {
  public:
	auto set_quad(graphics::Quad quad) -> void;

	auto setup() -> void override{};
	auto tick(Duration /*dt*/) -> void override {}
	auto render(std::vector<graphics::RenderObject>& out) const -> void override;

	graphics::DynamicPrimitive primitive{};
	graphics::UnlitMaterial material{};
};

class Quad : public PrimitiveRenderer {
  public:
	auto tick(Duration dt) -> void override;
};
} // namespace le::ui
