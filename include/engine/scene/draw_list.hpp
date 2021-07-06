#pragma once
#include <vector>
#include <engine/scene/draw_layer.hpp>
#include <engine/scene/drawable.hpp>

namespace le {
struct DrawList {
	DrawLayer layer;
	std::vector<Drawable> drawables;

	bool operator==(DrawList const& rhs) const noexcept { return layer == rhs.layer; }
	auto operator<=>(DrawList const& rhs) const noexcept { return layer <=> rhs.layer; }
};
} // namespace le
