#include <engine/assets/asset_store.hpp>

namespace le {
void AssetStore::update() {
	m_resources.update();
	for (auto& [_, store] : m_assets.storeMap) {
		store->update();
	}
}

void AssetStore::clear() {
	m_assets.storeMap.clear();
}
} // namespace le
