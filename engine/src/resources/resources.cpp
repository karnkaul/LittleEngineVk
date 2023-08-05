#include <le/core/logger.hpp>
#include <le/core/time.hpp>
#include <le/resources/resources.hpp>

namespace le {
namespace {
auto const g_log{logger::Logger{"Resources"}};
}

auto Resources::try_load(Uri const& uri, Asset& out) -> bool {
	auto delta_time = DeltaTime{};
	if (!out.try_load(uri)) {
		g_log.error("failed to load {}: '{}'", out.type_name(), uri.value());
		return false;
	}
	auto const dt = std::chrono::duration<float, std::milli>{delta_time()};
	g_log.info("[{:.1f}ms] '{}' {} loaded", dt.count(), uri.value(), out.type_name());
	return true;
}

auto Resources::store(Uri const& uri, std::unique_ptr<Asset> asset) -> Ptr<Asset> {
	auto* ret = set(uri, std::move(asset));
	if (ret != nullptr) { g_log.info("'{}' {} stored", uri.value(), asset->type_name()); }
	return ret;
}

auto Resources::contains(std::string_view const uri) const -> bool {
	auto lock = std::scoped_lock{m_mutex};
	return m_assets.contains(uri);
}

auto Resources::find(std::string_view const uri) const -> Ptr<Asset> {
	auto lock = std::scoped_lock{m_mutex};
	if (auto const it = m_assets.find(uri); it != m_assets.end()) { return it->second.get(); }
	return {};
}

auto Resources::clear() -> void {
	auto lock = std::scoped_lock{m_mutex};
	auto const count = m_assets.size();
	m_assets.clear();
	g_log.info("{} assets destroyed", count);
}

auto Resources::set(Uri uri, std::unique_ptr<Asset> asset) -> Ptr<Asset> {
	if (!asset || !uri) { return {}; }
	auto lock = std::scoped_lock{m_mutex};
	auto [it, _] = m_assets.insert_or_assign(std::move(uri), std::move(asset));
	return it->second.get();
}
} // namespace le
