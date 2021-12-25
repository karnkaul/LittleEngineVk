#pragma once
#include <type_traits>

namespace le::utils {
template <typename T, T Default>
struct Value {
	using type = T;
	static constexpr T default_v = Default;

	T value = Default;

	constexpr operator T const&() const noexcept { return value; }
	constexpr auto operator<=>(Value const&) const = default;
};
} // namespace le::utils
