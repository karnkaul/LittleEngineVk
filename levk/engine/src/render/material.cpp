#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/graphics/texture.hpp>

namespace le {
Opt<graphics::Texture const> TextureWrap::operator()(AssetStore const& store) const {
	if (m_data.uri != Hash()) {
		if (auto tex = store.find<Texture>(m_data.uri)) { return &*tex; }
	}
	return m_data.texture;
}
} // namespace le
