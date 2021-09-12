#pragma once
#include <core/std_types.hpp>

namespace le {
struct Codepoint {
	using type = u32;

	u32 value{};

	constexpr Codepoint(u32 value = {}) noexcept : value(value) {}
	constexpr operator u32() const noexcept { return value; }
	constexpr auto operator<=>(Codepoint const&) const = default;

	struct Range {
		static constexpr u32 first = 32;
		static constexpr u32 last = 127;
	};

	struct RangeEASCII : Range {
		static constexpr u32 last = 255;
	};

	template <typename T = RangeEASCII>
	struct Validate {
		[[nodiscard]] constexpr bool operator()(u32 codepoint) noexcept { return codepoint >= T::first && codepoint <= T::last; }
	};
};

constexpr Codepoint codepoint_blank_v = 10;
} // namespace le
