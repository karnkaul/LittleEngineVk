#pragma once
#include <functional>
#include <engine/scene/draw_layer.hpp>

namespace le {
struct LayerHasher {
	std::size_t operator()(DrawLayer const& gr) const noexcept {
		return std::hash<graphics::Pipeline const*>{}(gr.pipeline) ^ (std::hash<s64>{}(gr.order) << 1);
	}
};
} // namespace le
