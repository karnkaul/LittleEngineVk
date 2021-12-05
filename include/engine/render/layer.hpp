#pragma once
#include <core/std_types.hpp>
#include <graphics/render/pipeline_spec.hpp>

namespace le {
using PipelineState = graphics::PipelineSpec;

struct RenderLayer {
	PipelineState const* state{};
	s64 order = 0;

	bool operator==(RenderLayer const& rhs) const noexcept { return ((!state && !rhs.state) || (*state == *rhs.state)) && order == rhs.order; }
	auto operator<=>(RenderLayer const& rhs) const noexcept { return order <=> rhs.order; }

	Hash hash() const { return state ? state->hash() : Hash(); }

	struct Hasher {
		std::size_t operator()(RenderLayer const& list) const { return list.hash(); }
	};
};
} // namespace le
