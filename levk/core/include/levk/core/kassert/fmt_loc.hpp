#pragma once
#include <levk/core/kassert/src_loc.hpp>
#include <concepts>
#include <string_view>

namespace le {
struct FmtLoc {
	SrcLoc loc;
	std::string_view fmt;

	template <std::convertible_to<std::string_view> T>
	constexpr FmtLoc(T const& fmt, SrcLoc const& loc = SrcLoc::current()) : loc(loc), fmt(fmt) {}
};
} // namespace le
