#pragma once
#include <tuple>

namespace le {
template <std::size_t N, typename... T>
using ArrayMap = std::tuple<T...>[N];

template <typename Out, typename In, std::size_t N, typename... T>
constexpr Out const& mapped(ArrayMap<N, T...> const& map, In const& key, Out const& fallback = {}) noexcept {
	for (auto const& t : map) {
		if (std::get<In>(t) == key) {
			return std::get<Out>(t);
		}
	}
	return fallback;
}
} // namespace le
