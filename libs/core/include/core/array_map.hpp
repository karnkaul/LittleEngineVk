#pragma once
#include <type_traits>

namespace le {
template <typename A, typename B, std::size_t N>
struct ArrayMap {
	template <typename T, typename U>
	static constexpr bool combination_types_v = (std::is_same_v<A, T> && std::is_same_v<B, U>) || (std::is_same_v<A, U> && std::is_same_v<B, T>);

	struct Pair {
		A a;
		B b;
	};

	static constexpr auto blank = Pair{};

	Pair pairs[N];

	constexpr B const& operator[](A const& lhs) const noexcept { return get(lhs, blank.b); }
	constexpr A const& operator[](B const& lhs) const noexcept { return get(lhs, blank.a); }

	template <typename Out, typename In>
		requires(combination_types_v<In, Out>)
	Out const& get(In const& in, Out const& fallback) const noexcept {
		for (auto const& [a, b] : pairs) {
			if constexpr (std::is_same_v<In, A>) {
				if (a == in) { return b; }
			} else {
				if (b == in) { return a; }
			}
		}
		return fallback;
	}
};
} // namespace le
