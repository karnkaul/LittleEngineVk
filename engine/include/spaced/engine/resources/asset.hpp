#pragma once
#include <djson/json.hpp>
#include <spaced/engine/core/named_type.hpp>
#include <spaced/engine/vfs/uri.hpp>
#include <cstdint>
#include <vector>

namespace spaced {
class Asset : public NamedType {
  public:
	[[nodiscard]] auto type_name() const -> std::string_view override { return "Asset"; }
	[[nodiscard]] virtual auto try_load(Uri const& uri) -> bool = 0;

  protected:
	[[nodiscard]] auto read_bytes(Uri const& uri) const -> std::vector<std::uint8_t>;
	[[nodiscard]] auto read_string(Uri const& uri) const -> std::string;
	[[nodiscard]] auto read_json(Uri const& uri) const -> dj::Json;
};
} // namespace spaced
