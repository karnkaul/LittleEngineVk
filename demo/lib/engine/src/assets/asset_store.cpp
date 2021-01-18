#include <engine/assets/asset_store.hpp>

namespace le {
AssetStore::~AssetStore() {
	clear();
}

void AssetStore::update() {
	static constexpr u32 maxPasses = 10;
	bool bIdle = false;
	u64 total = 0;
	u32 pass = 0;
	auto lock = m_mrsw.lock<std::shared_lock>();
	for (; pass < maxPasses && (pass == 0 || !bIdle); ++pass) {
		m_resources.update();
		u64 reloaded = 0;
		for (auto& [_, store] : m_assets.storeMap) {
			reloaded += store->update(*this);
		}
		bIdle = reloaded == 0;
		total += reloaded;
		if (!bIdle) {
			conf::g_log.log(dl::level::debug, 2, "[Assets] [{}] Update pass: reloaded [{}]", pass, reloaded);
		}
	}
	if (!bIdle && pass == maxPasses) {
		conf::g_log.log(dl::level::warning, 0, "[Assets] Exceeded max update passes [{}], bailing out... Asset(s) stuck in reload loops?", maxPasses);
	} else if (total > 0) {
		conf::g_log.log(dl::level::info, 1, "[Assets] [{}] Reloads completed in [{}] passes", total, pass);
	}
}

void AssetStore::clear() {
	auto lock = m_mrsw.lock<std::unique_lock>();
	// Reset infos and assets before delegates (may contain OnModified tokens)
	m_assets.storeMap.clear();
	// Clear delegates
	m_onModified.clear();
	m_resources.clear();
}
} // namespace le