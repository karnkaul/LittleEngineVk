#include <spaced/engine/core/logger.hpp>
#include <spaced/engine/resources/asset.hpp>
#include <spaced/engine/vfs/vfs.hpp>

namespace spaced {
namespace {
auto const g_log{logger::Logger{"Asset"}};
}

// NOLINTNEXTLINE
auto Asset::read_bytes(Uri const& uri) const -> std::vector<std::uint8_t> { return vfs::read_bytes(uri); }
// NOLINTNEXTLINE
auto Asset::read_string(Uri const& uri) const -> std::string { return vfs::read_string(uri); }

auto Asset::read_json(Uri const& uri) const -> dj::Json {
	auto ret = dj::Json::parse(read_string(uri));
	if (!ret) { return {}; }
	auto const asset_type = ret["asset_type"].as_string();
	if (asset_type != type_name()) {
		g_log.error("asset type mismatch! expected: '{}', read: '{}'", type_name(), asset_type);
		return {};
	}
	return ret;
}
} // namespace spaced
