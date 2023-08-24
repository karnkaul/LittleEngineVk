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

auto to_string(Version const& version) -> std::string;
} // namespace le
