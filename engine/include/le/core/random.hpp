#pragma once
#include <cassert>
#include <concepts>
#include <random>

namespace le {
struct Random {
	std::default_random_engine engine{std::random_device{}()};

	template <std::integral T>
	auto in_range(T lo, T hi) -> T {
		return std::uniform_int_distribution<T>{lo, hi}(engine);
	}

	template <std::floating_point T>
	auto in_range(T lo, T hi) -> T {
		return std::uniform_real_distribution<T>{lo, hi}(engine);
	}

	auto index(std::size_t container_size) -> std::size_t {
		assert(container_size > 0u);
		return in_range<std::size_t>(0u, container_size - 1);
	}

	static auto default_instance() -> Random& {
		static auto ret = Random{};
		return ret;
	}
};

template <typename T>
auto random_range(T lo, T hi) -> T {
	return Random::default_instance().in_range(lo, hi);
}

inline auto random_index(std::size_t container_size) -> std::size_t { return Random::default_instance().index(container_size); }
} // namespace le
