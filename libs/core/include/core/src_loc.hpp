#pragma once
#include <core/std_types.hpp>

// clang-format off
#define SRC_LOC() ::le::src_loc{__FILE__, __func__, __LINE__}
// clang-format on

namespace le {
struct src_loc final {
	char const* szFile = nullptr;
	char const* szFunc = nullptr;
	std::uint_least32_t line_num = 0;

	constexpr std::uint_least32_t line() const noexcept { return line_num; }

	constexpr char const* file_name() const noexcept { return szFile; }

	constexpr char const* function_name() const noexcept { return szFunc; }
};
} // namespace le
