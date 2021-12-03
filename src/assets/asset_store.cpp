#include <engine/assets/asset_store.hpp>

namespace le {
void AssetStore::update() {
	ktl::unique_tlock<detail::TAssets> lock(m_assets);
	m_resources.update();
	u64 reloaded = 0;
	for (auto& [_, store] : lock->storeMap) { reloaded += store->update(*this); }
	if (reloaded > 0) { utils::g_log.log(dl::level::info, 1, "[Assets] [{}] Reloads completed", reloaded); }
}

void AssetStore::clear() {
	// clear assets
	ktl::unique_tlock<detail::TAssets>(m_assets)->storeMap.clear();
	// clear delegates
	ktl::tlock(m_onModified)->clear();
	// clear resources
	m_resources.clear();
}

AssetStore::Index AssetStore::index(std::string_view filter) const {
	ktl::shared_tlock<detail::TAssets const> lock(m_assets);
	Index ret;
	ret.map.reserve(lock->storeMap.size());
	for (auto const& [_, map] : lock->storeMap) {
		if (auto vec = map->uris(filter); !vec.empty()) { ret.map.push_back({map->typeName(), std::move(vec)}); }
	}
	return ret;
}

AssetStore::TypeMap AssetStore::typeMap(std::size_t typeHash, std::string_view filter) const {
	ktl::shared_tlock<detail::TAssets const> lock(m_assets);
	if (auto it = lock->storeMap.find(typeHash); it != lock->storeMap.end()) { return {it->second->typeName(), it->second->uris(filter)}; }
	return {};
}
} // namespace le
