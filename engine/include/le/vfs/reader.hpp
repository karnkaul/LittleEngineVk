#pragma once
#include <le/vfs/uri.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace le {
class Reader {
  public:
	Reader() = default;
	Reader(Reader const&) = delete;
	Reader(Reader&&) = delete;
	auto operator=(Reader const&) -> Reader& = delete;
	auto operator=(Reader&&) -> Reader& = delete;

	virtual ~Reader() = default;

	[[nodiscard]] static auto as_string(std::span<std::uint8_t const> bytes) -> std::string_view {
		// NOLINTNEXTLINE
		return {reinterpret_cast<char const*>(bytes.data()), bytes.size()};
	}

	[[nodiscard]] virtual auto read_bytes(Uri const& uri) -> std::vector<std::uint8_t> = 0;
	[[nodiscard]] virtual auto read_string(Uri const& uri) -> std::string { return std::string{as_string(read_bytes(uri))}; }
};
} // namespace le
