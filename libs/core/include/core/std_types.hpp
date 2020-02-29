#pragma once
#include <cstdint>
#include <cstddef>
#if defined(__MINGW32__)
#define __STDC_FORMAT_MACROS
#endif
#include <utility>
#include <vector>

#define ARR_SIZE(arr) sizeof(arr) / sizeof(arr[0])

namespace le
{
using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using f32 = float;
using f64 = double;
using size_t = std::size_t;
using bytearray = std::vector<std::byte>;

template <typename... Ts>
constexpr bool alwaysFalse = false;

template <typename... Ts>
constexpr bool alwaysTrue = true;

template <typename Type, typename Code = bool>
using TResult = std::pair<Code, Type>;
} // namespace le
