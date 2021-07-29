#include <engine/assets/asset_store.hpp>

namespace le {
void AssetStore::update() {
	static constexpr u32 maxPasses = 10;
	bool idle = false;
	u64 total = 0;
	u32 pass = 0;
	ktl::shared_tlock<detail::TAssets> lock(m_assets);
	for (; pass < maxPasses && (pass == 0 || !idle); ++pass) {
		m_resources.update();
		u64 reloaded = 0;
		for (auto& [_, store] : lock->storeMap) { reloaded += store->update(*this); }
		idle = reloaded == 0;
		total += reloaded;
		if (!idle) { utils::g_log.log(dl::level::debug, 2, "[Assets] [{}] Update pass: reloaded [{}]", pass, reloaded); }
	}
	if (!idle && pass == maxPasses) {
		utils::g_log.log(dl::level::warning, 0, "[Assets] Exceeded max update passes [{}], bailing out... Asset(s) stuck in reload loops?", maxPasses);
	} else if (total > 0) {
		utils::g_log.log(dl::level::info, 1, "[Assets] [{}] Reloads completed in [{}] passes", total, pass);
	}
}

void AssetStore::clear() {
	// clear assets
	ktl::unique_tlock<detail::TAssets>(m_assets)->storeMap.clear();
	// clear delegates
	ktl::tlock(m_onModified)->clear();
	// clear resources
	m_resources.clear();
}
} // namespace le
