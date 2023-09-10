#pragma once
#include <le/scene/ui/primitive_renderer.hpp>

namespace le::ui {
class Quad : public PrimitiveRenderer {
  public:
	using PrimitiveRenderer::PrimitiveRenderer;

	auto tick(Duration dt) -> void override;

	float corner_radius{5.0f};
};
} // namespace le::ui
