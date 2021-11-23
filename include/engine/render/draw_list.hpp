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

struct DrawList2 {
	DrawGroup group;
	std::vector<Drawable> drawables;

	auto operator<=>(DrawList2 const& rhs) const noexcept { return group <=> rhs.group; }

	Hash hash() const { return group.state.hash(); }

	struct Hasher {
		std::size_t operator()(DrawList2 const& list) const { return list.hash(); }
	};
};
} // namespace le
