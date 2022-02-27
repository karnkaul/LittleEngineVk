#pragma once
#include <levk/engine/render/render_order.hpp>
#include <levk/graphics/render/draw_list.hpp>
#include <levk/graphics/render/pipeline.hpp>

namespace le {
struct RenderList {
	graphics::Pipeline pipeline;
	graphics::DrawList drawList;
	RenderOrder order = RenderOrder::eDefault;

	auto operator<=>(RenderList const& rhs) const { return order <=> rhs.order; }
};
} // namespace le
