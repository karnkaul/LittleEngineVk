#pragma once
#include <concepts>

namespace le {
template <std::integral Type>
[[nodiscard]] constexpr auto increment_wrapped(Type const value, Type const one_past_max) {
	return (value + Type(1)) % one_past_max;
}

template <std::integral Type>
[[nodiscard]] constexpr auto decrement_wrapped(Type const value, Type const one_past_max) {
	return value == Type(0) ? one_past_max - Type(1) : value - Type(1);
}

template <typename StorageT>
struct Wrap {
	StorageT data{};
	std::size_t offset{};

	[[nodiscard]] constexpr auto get_current() const -> decltype(auto) { return data[offset]; }
	[[nodiscard]] constexpr auto get_current() -> decltype(auto) { return data[offset]; }

	[[nodiscard]] constexpr auto get_previous() const -> decltype(auto) { return data[decrement_wrapped(offset, data.size())]; }
	[[nodiscard]] constexpr auto get_previous() -> decltype(auto) { return data[decrement_wrapped(offset, data.size())]; }

	constexpr auto advance() -> void { offset = increment_wrapped(offset, data.size()); }
};
} // namespace le
