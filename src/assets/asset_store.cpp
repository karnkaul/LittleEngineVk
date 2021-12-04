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

AssetStore::Index AssetStore::index(Span<std::size_t const> types, std::string_view filter) const {
	ktl::shared_tlock<detail::TAssets const> lock(m_assets);
	Index ret;
	ret.maps.reserve(lock->storeMap.size());
	for (auto const& [hash, map] : lock->storeMap) {
		if (types.empty() || std::find(types.begin(), types.end(), hash) != types.end()) {
			if (auto vec = map->uris(filter); !vec.empty()) {
				Type type{map->typeName(), hash};
				ret.maps.push_back({type, std::move(vec)});
			}
		}
	}
	return ret;
}
} // namespace le
