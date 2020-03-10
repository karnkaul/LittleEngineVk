#pragma once
#if defined(__MINGW32__)
#define __STDC_FORMAT_MACROS
#endif
#include <cstdint>
#include <cstddef>
#include <limits>
#include <utility>
#include <type_traits>
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

template <typename T>
struct FalseType final : std::false_type
{
};

template <typename T>
struct TrueType final : std::true_type
{
};

template <typename Type, typename Code = bool>
using TResult = std::pair<Code, Type>;

template <typename T>
constexpr T maxVal()
{
	static_assert(std::is_arithmetic_v<T>, "T must be numeric!");
	return (std::numeric_limits<T>::max)();
}
} // namespace le
