#include <engine/assets/asset_store.hpp>
#include <engine/scene/material.hpp>
#include <graphics/texture.hpp>

namespace le {
graphics::Texture const* IMaterial::Helper::texture(Hash uri) const {
	if (auto it = m_textures.find(uri); it != m_textures.end()) { return it->second; }
	if (auto tex = m_store->find<Texture>(uri)) { return m_textures.emplace(uri, &*tex).first->second; }
	return {};
}
} // namespace le
