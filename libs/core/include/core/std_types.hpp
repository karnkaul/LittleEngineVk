#pragma once
#if defined(__MINGW32__)
#define __STDC_FORMAT_MACROS
#endif
#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <utility>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#if defined(near)
#undef near
#endif
#if defined(far)
#undef far
#endif
#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

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
using bytearray = std::vector<std::byte>;

using namespace std::string_view_literals;

template <typename E, typename T = std::string_view, std::size_t N = (std::size_t)E::eCOUNT_>
using EnumArray = std::array<T, N>;

template <typename... Ts>
constexpr bool alwaysFalse = false;

template <typename... Ts>
constexpr bool alwaysTrue = true;

///
/// \brief Obtain the number of elements in a stack array
///
template <typename T, std::size_t N>
constexpr std::size_t arraySize(T (&)[N])
{
	return N;
}

///
/// \brief Obtain the max value for `T`
///
template <typename T>
constexpr T maxVal()
{
	static_assert(std::is_arithmetic_v<T>, "T must be arithemtic!");
	return std::numeric_limits<T>::max();
}

///
/// \brief Ultra-light reference wrapper
///
template <typename T>
struct Ref
{
	using type = T;

	T* pPtr;

	constexpr Ref(T& t) noexcept : pPtr(&t) {}

	constexpr operator T&() const
	{
		return *pPtr;
	}
};

///
/// \brief Structured Binding of a payload and a `bool` (indicating the result of an operation)
///
template <typename T>
struct TResult
{
	using type = T;

	T payload = {};
	bool bResult = false;

	constexpr TResult() = default;
	constexpr TResult(T&& payload) : payload(std::move(payload)), bResult(true) {}
	constexpr TResult(T&& payload, bool bResult) : payload(std::move(payload)), bResult(bResult) {}

	constexpr operator bool() const;
	constexpr T const& operator*() const;
	constexpr T& operator*();
	constexpr T const* operator->() const;
	constexpr T* operator->();
};

template <>
struct TResult<void>
{
	using type = void;

	bool bResult = false;

	constexpr TResult() noexcept = default;
	constexpr TResult(bool bResult) noexcept : bResult(bResult) {}

	constexpr operator bool() const;
};

template <typename T>
constexpr TResult<T>::operator bool() const
{
	return bResult;
}

template <typename T>
constexpr T const& TResult<T>::operator*() const
{
	return payload;
}

template <typename T>
constexpr T& TResult<T>::operator*()
{
	return payload;
}

template <typename T>
constexpr T const* TResult<T>::operator->() const
{
	return &payload;
}

template <typename T>
constexpr T* TResult<T>::operator->()
{
	return &payload;
}

inline constexpr TResult<void>::operator bool() const
{
	return bResult;
}

template <typename T>
constexpr inline bool operator==(Ref<T> lhs, Ref<T> rhs)
{
	return lhs.pPtr == rhs.pPtr;
}

template <typename T>
constexpr inline bool operator!=(Ref<T> lhs, Ref<T> rhs)
{
	return lhs.pPtr != rhs.pPtr;
}
} // namespace le
