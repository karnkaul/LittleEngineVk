#pragma once
#include <core/std_types.hpp>
#include <atomic>
#include <type_traits>

namespace le::utils {
template <typename T>
	requires std::is_convertible_v<T, f32>
struct Ratio {
	std::atomic<T> a{};
	std::atomic<T> b{};

	bool operator==(Ratio const&) const = default;

	Ratio& operator++() noexcept { return (++a, ++b, *this); }
	Ratio& operator++(int) noexcept { return (a++, b++, *this); }
	Ratio& operator--() noexcept { return (--a, --b, *this); }
	Ratio& operator--(int) noexcept { return (a--, b--, *this); }

	f32 ratio() const noexcept { return static_cast<f32>(a.load()) / static_cast<f32>(b.load()); }
};
} // namespace le::utils
