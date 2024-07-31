#pragma once
#include <compare>
#include <cstdint>

namespace levk {
/// \brief Semantic Version.
struct Version {
	std::uint32_t major{};
	std::uint32_t minor{};
	std::uint32_t patch{};

	auto operator<=>(Version const&) const = default;
};
} // namespace levk
