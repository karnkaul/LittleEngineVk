#pragma once
#include <cstdint>
#include <string>

namespace le {
struct Version {
	std::uint8_t major{};
	std::uint8_t minor{};
	std::uint8_t patch{};

	[[nodiscard]] auto to_string() const -> std::string;
	[[nodiscard]] auto to_vk_version() const -> std::uint32_t;

	auto operator<=>(Version const&) const = default;
};

constexpr Version const build_version_v{LE_VERSION_MAJOR, LE_VERSION_MINOR, LE_VERSION_PATCH};

auto to_string(Version const& version) -> std::string;
} // namespace le
