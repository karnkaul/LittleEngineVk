#pragma once
#include <string>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_ENSURES)
#define LEVK_ENSURES
#endif
#endif

namespace le {
#if defined(LEVK_ENSURES)
///
/// \brief Enables ensure(false, message) - prints error message, breaks if debugger attached
///
inline constexpr bool levk_ensures = true;
#else
inline constexpr bool levk_ensures = false;
#endif

///
/// \brief Trigger a break (if debugger present) / assert if predicate is false
///
void ensure(bool pred, std::string_view msg = {}, char const* fl = __builtin_FILE(), char const* fn = __builtin_FUNCTION(), int ln = __builtin_LINE());
} // namespace le
