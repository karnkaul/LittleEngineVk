#pragma once
#include <string>
#include "std_types.hpp"

#if defined(LEVK_DEBUG)
/**
 * Variable     : LEVK_ASSERTS
 * Description  : Enables ASSERT() macro - breaks when debugger attached
 */
#if !defined(LEVK_ASSERTS)
#define LEVK_ASSERTS
#endif
#endif

#if defined(LEVK_ASSERTS)
#define ASSERT(predicate, errorMessage) le::assertMsg(!!(predicate), errorMessage, __FILE__, __LINE__)
#else
#define ASSERT(disabled, _disabled)
#endif

namespace le
{
void assertMsg(bool predicate, std::string_view message, std::string_view fileName, u64 lineNumber);
} // namespace le
