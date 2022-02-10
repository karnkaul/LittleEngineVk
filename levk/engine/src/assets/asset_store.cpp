#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/assets/asset_store2.hpp>
#include <atomic>

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

auto AssetStore::onModified(Hash uri) -> OnModified::signal {
	ktl::klock lock(m_onModified);
	if (auto it = lock->find(uri); it != lock->end()) { return it->second.make_signal(); }
	return {};
}

std::size_t AssetStore::size() const { return ktl::klock(m_assets)->size(); }

void AssetStore::checkModified() {
	static std::atomic<bool> s_updating = false;
	EXPECT(!s_updating);
	s_updating = true;
	std::vector<Base*> dirty;
	u64 reloaded = 0;
	{
		ktl::klock lock(m_assets);
		m_resources.update();
		for (auto& [_, asset] : *lock) {
			EXPECT(asset);
			if (asset->dirty() && asset->doReload) { dirty.push_back(asset.get()); }
		}
	}
	for (auto base : dirty) {
		if (base->doReload(*base)) { ++reloaded; }
	}
	if (reloaded > 0) { logI(LC_LibUser, "[Assets] [{}] Reloads completed", reloaded); }
	EXPECT(s_updating);
	s_updating = false;
}

void AssetStore::clear() {
	// clear assets
	ktl::klock(m_assets)->clear();
	// clear resources
	m_resources.clear();
}

AssetStore::Index AssetStore::index(Span<Sign const> signs, std::string_view filter) const {
	ktl::klock lock(m_assets);
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

void AssetStore::modified(Hash uri) {
	auto lock = ktl::klock(m_onModified);
	if (auto it = lock->find(uri); it != lock->end()) { it->second(); }
}
} // namespace le
