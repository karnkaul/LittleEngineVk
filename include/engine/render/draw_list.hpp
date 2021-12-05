#pragma once
#include <core/hash.hpp>
#include <engine/render/drawable.hpp>
#include <graphics/render/pipeline.hpp>
#include <vector>

namespace le {
struct DrawList {
	graphics::Pipeline pipeline;
	std::vector<Drawable> drawables;
	s64 order = 0;

	auto operator<=>(DrawList const& rhs) const noexcept { return order <=> rhs.order; }
};
} // namespace le
