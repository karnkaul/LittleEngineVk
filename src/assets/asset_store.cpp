#include <engine/assets/asset_store.hpp>

namespace le {
bool AssetStore::exists(Hash uri) const noexcept { return ktl::shared_klock<TAssets const>(m_assets)->contains(uri); }

bool AssetStore::unload(Hash uri) {
	ktl::shared_klock<TAssets> lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		lock->erase(it);
		return true;
	}
	return false;
}

void AssetStore::update() {
	ktl::shared_klock<TAssets> lock(m_assets);
	m_resources.update();
	u64 reloaded = 0;
	for (auto& [_, asset] : *lock) {
		EXPECT(asset);
		if (asset->doUpdate && asset->doUpdate(asset.get())) { ++reloaded; }
	}
	if (reloaded > 0) { logI(LC_LibUser, "[Assets] [{}] Reloads completed", reloaded); }
}

void AssetStore::clear() {
	// clear assets
	ktl::unique_klock<TAssets>(m_assets)->clear();
	// clear resources
	m_resources.clear();
}

AssetStore::Index AssetStore::index(Span<Sign const> signs, std::string_view filter) const {
	ktl::shared_klock<TAssets const> lock(m_assets);
	std::unordered_map<Sign, std::vector<Base*>, Sign::hasher> mapped;
	for (auto const& [hash, asset] : *lock) {
		EXPECT(asset);
		if (!filter.empty() && asset->uri.find(filter) == std::string_view::npos) { continue; }
		if (!signs.empty() && std::find(signs.begin(), signs.end(), asset->sign) == signs.end()) { continue; }
		mapped[asset->sign].push_back(asset.get());
	}
	Index ret;
	ret.maps.reserve(mapped.size());
	for (auto& [sign, assets] : mapped) {
		Index::Map map;
		map.type = Index::Type{assets[0]->typeName, assets[0]->sign};
		map.uris.reserve(assets.size());
		for (auto const asset : assets) { map.uris.push_back(asset->uri); }
		ret.maps.push_back(std::move(map));
	}
	return ret;
}
} // namespace le
