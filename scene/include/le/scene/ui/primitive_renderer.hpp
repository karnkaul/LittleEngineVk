#pragma once
#include <le/scene/ui/renderable.hpp>

namespace le::ui {
class PrimitiveRenderer : public Renderable {
  public:
	using Renderable::Renderable;

	void set_quad(graphics::Quad quad);
	void set_texture(Ptr<graphics::Texture const> texture);

	void tick(Duration /*dt*/) override {}
	void render(std::vector<graphics::RenderObject>& out) const override;

	graphics::DynamicPrimitive primitive{};
	graphics::UnlitMaterial material{};
};
} // namespace le::ui
