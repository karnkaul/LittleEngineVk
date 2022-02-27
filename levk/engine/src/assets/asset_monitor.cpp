#include <levk/engine/assets/asset_monitor.hpp>
#include <filesystem>

namespace le {
namespace {
AssetMonitor::FTime ftime(std::string_view const path) {
	auto const p = std::filesystem::path(path);
	if (std::filesystem::is_regular_file(p)) { return std::filesystem::last_write_time(p); }
	return {};
}
} // namespace

AssetMonitor::Base::Base(Span<io::Path const> paths) {
	for (auto& path : paths) {
		auto str = path.generic_string();
		auto const ft = ftime(str);
		entries.push_back({std::move(str), ft});
	}
}

bool AssetMonitor::Base::modified() {
	bool ret{};
	for (auto& entry : entries) {
		if (auto const lm = ftime(entry.path); lm != entry.lastModified) {
			entry.lastModified = lm;
			ret = true;
		}
	}
	return ret;
}

std::size_t AssetMonitor::update(AssetStore const& store) {
	std::size_t ret{};
	for (auto it = m_monitors.begin(); it != m_monitors.end();) {
		if (!store.exists(it->first)) {
			it = m_monitors.erase(it);
		} else {
			++it;
		}
	}
	for (auto const& [uri, monitor] : m_monitors) {
		if (monitor->update(store, uri)) { ++ret; }
	}
	if (ret > 0U) { logI(LC_EndUser, "[Assets] [{}] monitors updated", ret); }
	return ret;
}
} // namespace le
