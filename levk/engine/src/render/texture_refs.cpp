#include <ktl/enumerate.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/render/texture_refs.hpp>
#include <levk/graphics/texture.hpp>

namespace le {
bool TextureRefs::fill(AssetStore const& store, graphics::MaterialTextures& out) const {
	for (auto const [hash, idx] : ktl::enumerate(textures.arr)) {
		if (hash != Hash()) {
			auto tex = store.find<graphics::Texture>(hash);
			if (!tex || !tex->ready()) { return false; }
			out[graphics::MatTexType(idx)] = tex;
		}
	}
	return true;
}
} // namespace le
