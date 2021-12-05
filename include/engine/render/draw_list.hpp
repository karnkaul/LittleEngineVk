#pragma once
#include <core/hash.hpp>
#include <engine/render/drawable.hpp>
#include <graphics/render/pipeline.hpp>
#include <vector>

namespace le {
class DescriptorUpdater;
class DescriptorMap;

struct DrawList : DrawBindable {
	std::vector<Drawable> drawables;
	graphics::Pipeline pipeline;
	s64 order = 0;

	auto operator<=>(DrawList const& rhs) const noexcept { return order <=> rhs.order; }
};
} // namespace le
