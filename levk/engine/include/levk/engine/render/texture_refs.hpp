#pragma once
#include <levk/core/hash.hpp>
#include <levk/graphics/draw_primitive.hpp>

namespace le {
class AssetStore;

struct TextureRefs {
	graphics::TMatTexArray<Hash> textures{};

	bool fill(AssetStore const& store, graphics::MaterialTextures& out) const;
};
} // namespace le
