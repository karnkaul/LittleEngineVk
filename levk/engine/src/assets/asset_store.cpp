#include <levk/engine/assets/asset_store.hpp>

namespace le {
bool AssetStore::exists(Hash uri) const { return ktl::klock(m_assets)->contains(uri); }

bool AssetStore::unload(Hash uri) {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		lock->erase(it);
		return true;
	}
	return false;
}

std::size_t AssetStore::size() const { return ktl::klock(m_assets)->size(); }

void AssetStore::clear() { ktl::klock(m_assets)->clear(); }

AssetStore::Index AssetStore::index(Span<Sign const> signs, std::string_view filter) const {
	ktl::klock lock(m_assets);
	std::unordered_map<Sign, std::vector<Base*>, std::hash<Sign::type>> mapped;
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
