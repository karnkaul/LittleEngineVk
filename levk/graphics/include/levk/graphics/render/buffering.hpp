#pragma once
#include <levk/core/std_types.hpp>

namespace le::graphics {
enum struct Buffering : u32 { eNone = 0U, eSingle = 1U, eDouble = 2U, eTriple = 3U };

constexpr Buffering& operator++(Buffering& b) noexcept { return (b = Buffering{u32(b) + 1}, b); }
constexpr Buffering operator+(Buffering a, Buffering b) noexcept { return Buffering{u32(a) + u32(b)}; }
} // namespace le::graphics
