#pragma once
#include <core/std_types.hpp>

namespace le::graphics {
struct Buffering {
	u8 value{};

	constexpr auto operator<=>(Buffering const&) const = default;
};

constexpr Buffering operator""_B(unsigned long long value) noexcept { return Buffering{static_cast<u8>(value)}; }

constexpr auto doubleBuffer = 2_B;
constexpr auto tripleBuffer = 3_B;
} // namespace le::graphics
