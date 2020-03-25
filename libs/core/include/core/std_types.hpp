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

template <typename T>
struct TResult
{
	using type = T;

	T payload;
	bool bResult = false;

	TResult() = default;
	TResult(T&& payload) : payload(std::forward<T&&>(payload)), bResult(true) {}
	TResult(T&& payload, bool bResult) : payload(std::forward<T&&>(payload)), bResult(bResult) {}
};

template <typename T, typename = std::enable_if_t<std::is_array_v<T>>>
constexpr size_t arraySize(T const& arr)
{
	return sizeof(arr) / sizeof(arr[0]);
}

template <typename T, typename = std::enable_if<std::is_arithmetic_v<T>>>
constexpr T maxVal()
{
	return std::numeric_limits<T>::max();
}
} // namespace le
