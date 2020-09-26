#pragma once
#include <type_traits>

namespace le
{
///
/// \brief True if all B... are true
///
template <bool... B>
constexpr bool require_v = (B && ...);

///
/// \brief True if any B... are true
///
template <bool... B>
constexpr bool either_v = (B || ...);

template <typename T, bool... B>
using require_t = std::enable_if_t<require_v<B...>, T>;
template <bool... B>
using require = std::enable_if_t<require_v<B...>>;
///
/// \brief True if all B... are false
///
template <bool... B>
constexpr bool prohibit_v = (!B && ...);
///
/// \brief True if count of T... is equal to N
///
template <std::size_t N, typename... T>
constexpr bool size_eq_v = sizeof...(T) == N;
///
/// \brief True if count of T... is less than N
///
template <std::size_t N, typename... T>
constexpr bool size_lt_v = sizeof...(T) < N;
///
/// \brief True if count of T... is less than or equal to N
///
template <std::size_t N, typename... T>
constexpr bool size_le_v = sizeof...(T) <= N;
} // namespace le
