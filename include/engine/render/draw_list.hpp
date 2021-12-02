#pragma once
#include <core/hash.hpp>
#include <engine/render/draw_group.hpp>
#include <engine/render/drawable.hpp>
#include <vector>

namespace le {
struct DrawList {
	DrawGroup group;
	std::vector<Drawable> drawables;

	auto operator<=>(DrawList const& rhs) const noexcept { return group <=> rhs.group; }

	Hash hash() const { return group.hash(); }

	struct Hasher {
		std::size_t operator()(DrawList const& list) const { return list.hash(); }
	};
};

struct DrawList2 {
	DrawGroup group;
	std::vector<Drawable2> drawables;

	auto operator<=>(DrawList2 const& rhs) const noexcept { return group <=> rhs.group; }

	Hash hash() const { return group.hash(); }
};
} // namespace le
