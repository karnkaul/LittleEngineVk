#pragma once
#include <array>
#include <format>
#include <span>

namespace le {
template <typename Type, Type Multiplier>
auto format_unit(Type const value, std::span<std::string_view const> suffix_map, int precision = 3) -> std::string {
	if (suffix_map.empty() || Multiplier == Type(0)) { return std::format("{}", value); }

	static constexpr auto m = static_cast<float>(Multiplier);

	auto const negative = value < Type(0);
	auto f = std::abs(static_cast<float>(value));
	auto i = std::size_t{};
	while (f > Multiplier && i + 1 < suffix_map.size()) {
		f = f / m;
		++i;
	}

	auto const fmt = std::format("{{}}{{:.{}}}{{}}", precision);
	return std::vformat(fmt, std::make_format_args((negative ? "-" : ""), f, suffix_map[i]));
}

template <typename Type>
auto format_bytes(Type const count, int precision = 3) -> std::string {
	using namespace std::string_view_literals;
	static constexpr auto suffix_map_v = std::array{
		"B"sv,
		"KiB"sv,
		"MiB"sv,
		"GiB"sv,
	};
	static constexpr auto multiplier_v = Type(1024);
	return format_unit<Type, multiplier_v>(count, suffix_map_v, precision);
}
} // namespace le
