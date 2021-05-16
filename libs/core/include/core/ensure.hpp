#pragma once
#include <string>
#include <core/src_loc.hpp>
#include <core/std_types.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_ENSURES)
#define LEVK_ENSURES
#endif
#endif

#define ENSURE(predicate, message) ::le::ensure(!!(predicate), message, SRC_LOC())

namespace le {
#if defined(LEVK_ENSURES)
///
/// \brief Enables ensure(false, message) - prints error message, breaks if debugger attached
///
inline constexpr bool levk_ensures = true;
#else
inline constexpr bool levk_ensures = false;
#endif

void ensureImpl(std::string_view message, src_loc const& sl);

///
/// \brief Trigger a break (if debugger present) / assert if predicate is false
///
inline void ensure([[maybe_unused]] bool predicate, [[maybe_unused]] std::string_view message, [[maybe_unused]] src_loc const& sl) {
	if constexpr (levk_ensures) {
		if (!predicate) { ensureImpl(message, sl); }
	}
}
} // namespace le
