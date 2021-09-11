#pragma once
#if defined(__MINGW32__)
#define __STDC_FORMAT_MACROS
#endif
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
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

#if defined(LEVK_DEBUG)
inline constexpr bool levk_debug = true;
#else
inline constexpr bool levk_debug = false;
#endif

#if defined(LEVK_PRE_RELEASE)
inline constexpr bool levk_pre_release = true;
#else
inline constexpr bool levk_pre_release = false;
#endif

namespace le {
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

template <typename Enum, typename Ty, std::size_t N = (std::size_t)Enum::eCOUNT_>
	requires std::is_enum_v<Enum>
struct EnumArray {
	using type = Ty;

	type arr[N] = {};

	constexpr type const& operator[](Enum e) const noexcept { return arr[(std::size_t)e]; }
	constexpr type& operator[](Enum e) noexcept { return arr[(std::size_t)e]; }
};

template <typename...>
constexpr bool false_v = false;

template <typename...>
constexpr bool true_v = true;

///
/// \brief Obtain the number of elements in a stack array
///
template <typename T, std::size_t N>
constexpr std::size_t arraySize(T const (&)[N]) noexcept {
	return N;
}

///
/// \brief Convenience base type with deleted copy semantics
///
struct NoCopy {
	constexpr NoCopy() = default;
	constexpr NoCopy(NoCopy&&) = default;
	constexpr NoCopy& operator=(NoCopy&&) = default;
	NoCopy(NoCopy const&) = delete;
	NoCopy& operator=(NoCopy const&) = delete;
};

///
/// \brief Convenience base type with deleted movr/copy semantics
///
struct Pinned {
	constexpr Pinned() = default;
	Pinned(Pinned&&) = delete;
	Pinned& operator=(Pinned&&) = delete;
	Pinned(Pinned const&) = delete;
	Pinned& operator=(Pinned const&) = delete;
};
} // namespace le
