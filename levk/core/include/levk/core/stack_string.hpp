#pragma once
#include <fmt/format.h>
#include <cassert>

namespace le {
template <std::size_t Capacity>
struct StackString {
	static constexpr auto capacity_v = Capacity;

	char buf[Capacity + 1]{};
	std::size_t size{};

	StackString() = default;

	template <std::size_t N>
	constexpr StackString(char const (&str)[N]) {
		static_assert(N < Capacity);
		for (std::size_t i = 0; i < N; ++i) { buf[i] = str[i]; }
		size = N;
	}

	template <typename... Args>
	constexpr StackString(std::string_view const fmt, Args const&... args) {
		size = fmt::format_to(buf, fmt::runtime(fmt), args...) - buf;
		assert(size < Capacity);
	}

	constexpr std::string_view get() const { return buf; }
	constexpr operator std::string_view() const { return get(); }
	constexpr char const* data() const { return buf; }

	template <std::size_t N>
	constexpr StackString<Capacity>& operator+=(StackString<N> const& rhs) {
		size = fmt::format_to(buf + size, "{}", rhs.get()) - buf;
		assert(size < Capacity);
		return *this;
	}
};
} // namespace le
