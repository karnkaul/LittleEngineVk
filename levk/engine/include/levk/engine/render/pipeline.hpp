#pragma once
#include <ktl/fixed_vector.hpp>
#include <levk/engine/render/layer.hpp>
#include <compare>
#include <string>

namespace le {
struct RenderPipeline {
	ktl::fixed_vector<std::string, 4> shaderURIs;
	RenderLayer layer;

	bool operator==(RenderPipeline const& rhs) const;

	struct OrderCompare {
		std::strong_ordering operator()(RenderPipeline const& lhs, RenderPipeline const& rhs) const { return lhs.layer.order <=> rhs.layer.order; }
	};
	struct Hasher {
		std::size_t operator()(RenderPipeline const& rp) const;
	};
};
} // namespace le
