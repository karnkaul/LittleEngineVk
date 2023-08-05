#pragma once
#include <le/graphics/primitive.hpp>
#include <le/scene/ui/view.hpp>

namespace le::ui {
class Renderable : public View {
  public:
	static constexpr auto pipeline_state_v{graphics::PipelineState{.depth_test_write = 0}};

	auto render_to(std::vector<RenderObject>& out, NotNull<Material const*> material, NotNull<Primitive const*> primitive) const -> void;

	[[nodiscard]] auto get_tint() const -> graphics::Rgba { return render_instance.tint; }
	auto set_tint(graphics::Rgba tint) const -> void { render_instance.tint = tint; }

	graphics::PipelineState pipeline_state{pipeline_state_v};
	float z_index{0.0f};

  protected:
	mutable graphics::RenderInstance render_instance{};
};
} // namespace le::ui
