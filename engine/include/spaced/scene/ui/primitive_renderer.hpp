#pragma once
#include <spaced/graphics/primitive.hpp>
#include <spaced/scene/ui/view.hpp>

namespace spaced::ui {
class PrimitiveRenderer : public View {
  public:
	static constexpr auto pipeline_state_v{graphics::PipelineState{.depth_test_write = 0}};

	auto set_quad(graphics::Quad quad) -> void;

	auto render_tree(Rect2D<> const& parent_frame, std::vector<graphics::RenderObject>& out) const -> void override;

	graphics::DynamicPrimitive primitive{};
	graphics::UnlitMaterial material{};
	std::vector<graphics::RenderInstance> instances{};
	graphics::PipelineState pipeline_state{pipeline_state_v};
};
} // namespace spaced::ui
