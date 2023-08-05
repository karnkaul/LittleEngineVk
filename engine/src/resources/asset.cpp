#include <le/core/logger.hpp>
#include <le/resources/asset.hpp>
#include <le/vfs/vfs.hpp>

namespace le {
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
} // namespace le
