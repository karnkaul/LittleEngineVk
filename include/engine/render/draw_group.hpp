#pragma once
#include <core/std_types.hpp>
#include <graphics/render/pipeline_spec.hpp>

namespace le {
using PipelineState = graphics::PipelineSpec;

struct DrawGroup {
	PipelineState const* state{};
	s64 order = 0;

	bool operator==(DrawGroup const& rhs) const noexcept { return state == rhs.state && order == rhs.order; }
	auto operator<=>(DrawGroup const& rhs) const noexcept { return order <=> rhs.order; }

	Hash hash() const { return state ? state->hash() : Hash(); }

	struct Hasher {
		std::size_t operator()(DrawGroup const& list) const { return list.hash(); }
	};
};
} // namespace le
