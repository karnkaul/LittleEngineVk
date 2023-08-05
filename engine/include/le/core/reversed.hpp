#pragma once
#include <iterator>

namespace le {
template <typename It>
struct Reversed {
	It first{};
	It last{};

	constexpr auto begin() const { return std::reverse_iterator<It>{first}; }
	constexpr auto end() const { return std::reverse_iterator<It>{last}; }
};

template <typename It>
constexpr auto reversed(It first, It last) {
	return Reversed<It>{first, last};
}

template <typename ContainerT>
constexpr auto reversed(ContainerT&& container) {
	return reversed(std::end(container), std::begin(container));
}
} // namespace le
