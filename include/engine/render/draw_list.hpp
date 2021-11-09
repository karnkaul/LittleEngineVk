#pragma once
#include <core/hash.hpp>
#include <engine/render/draw_layer.hpp>
#include <engine/render/drawable.hpp>
#include <vector>

namespace le {
struct DrawList {
	DrawLayer layer;
	std::vector<Drawable> drawables;
	Hash variant;

	bool operator==(DrawList const& rhs) const noexcept { return layer == rhs.layer && variant == rhs.variant; }
	auto operator<=>(DrawList const& rhs) const noexcept { return layer <=> rhs.layer; }
};
} // namespace le
