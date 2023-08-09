#pragma once
#include <djson/json.hpp>
#include <le/core/named_type.hpp>
#include <le/vfs/uri.hpp>
#include <cstdint>
#include <vector>

namespace le {
class Asset : public NamedType {
  public:
	[[nodiscard]] auto type_name() const -> std::string_view override { return "Asset"; }
	[[nodiscard]] virtual auto try_load(Uri const& uri) -> bool = 0;

	[[nodiscard]] static auto get_asset_type(dj::Json const& json) -> std::string_view { return json["asset_type"].as_string(); }
	[[nodiscard]] static auto get_asset_type(Uri const& uri) -> std::string_view;

  protected:
	[[nodiscard]] auto read_bytes(Uri const& uri) const -> std::vector<std::byte>;
	[[nodiscard]] auto read_string(Uri const& uri) const -> std::string;
	[[nodiscard]] auto read_json(Uri const& uri) const -> dj::Json;
};
} // namespace le
