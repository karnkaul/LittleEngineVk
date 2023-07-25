#pragma once
#include <string_view>

namespace spaced {
constexpr bool debug_v =
#if defined(SPACED_DEBUG)
	true;
#else
	false;
#endif

auto build_version() -> std::string_view;
} // namespace spaced
