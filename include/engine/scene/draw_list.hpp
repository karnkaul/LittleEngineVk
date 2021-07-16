#pragma once
#include <vector>
#include <core/hash.hpp>
#include <engine/scene/draw_layer.hpp>
#include <engine/scene/drawable.hpp>

namespace le {
struct DrawList {
	DrawLayer layer;
	std::vector<Drawable> drawables;
	Hash variant;

	bool operator==(DrawList const& rhs) const noexcept { return layer == rhs.layer && variant == rhs.variant; }
	auto operator<=>(DrawList const& rhs) const noexcept { return layer <=> rhs.layer; }
};
} // namespace le
