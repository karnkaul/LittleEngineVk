#include <spaced/engine/core/logger.hpp>
#include <spaced/engine/resources/resources.hpp>

namespace spaced {
namespace {
auto const g_log{logger::Logger{"Resources"}};
}

auto Resources::try_load(Uri const& uri, Asset& out) -> bool {
	if (!out.try_load(uri)) {
		g_log.error("failed to load {}: '{}'", out.type_name(), uri.value());
		return false;
	}
	return true;
}

auto Resources::store(Uri uri, std::unique_ptr<Asset> asset) -> Ptr<Asset> { return set(std::move(uri), std::move(asset), "stored"); }

auto Resources::contains(Uri const& uri) const -> bool {
	auto lock = std::scoped_lock{m_mutex};
	return m_assets.contains(uri);
}

auto Resources::find(Uri const& uri) const -> Ptr<Asset> {
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

auto Resources::set(Uri uri, std::unique_ptr<Asset> asset, std::string_view log_suffix) -> Ptr<Asset> {
	if (!asset || !uri) { return {}; }
	auto lock = std::scoped_lock{m_mutex};
	g_log.info("'{}' {} {}", uri.value(), asset->type_name(), log_suffix);
	auto [it, _] = m_assets.insert_or_assign(std::move(uri), std::move(asset));
	return it->second.get();
}
} // namespace spaced
