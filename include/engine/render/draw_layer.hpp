#pragma once
#include <core/std_types.hpp>
#include <graphics/render/pipeline_spec.hpp>

namespace le {
namespace graphics {
class Pipeline;
}

struct DrawLayer {
	graphics::Pipeline* pipeline{};
	s64 order = 0;

	bool operator==(DrawLayer const& rhs) const noexcept = default;
	auto operator<=>(DrawLayer const& rhs) const noexcept { return order <=> rhs.order; }
};

using PipelineState = graphics::PipelineSpec;

struct DrawGroup {
	PipelineState state;
	s64 order = 0;

	bool operator==(DrawGroup const& rhs) const noexcept { return state == rhs.state && order == rhs.order; }
	auto operator<=>(DrawGroup const& rhs) const noexcept { return order <=> rhs.order; }

	Hash hash() const { return state.hash(); }

	struct Hasher {
		std::size_t operator()(DrawGroup const& list) const { return list.hash(); }
	};
};
} // namespace le
