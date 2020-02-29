#pragma once
#include <string>
#include "std_types.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LE3D_ASSERTS)
#define LE3D_ASSERTS
#endif
#endif

#if defined(LE3D_ASSERTS)
#define ASSERT(predicate, errorMessage) le::assertMsg(!!(predicate), errorMessage, __FILE__, __LINE__)
#else
#define ASSERT(disabled, _disabled)
#endif

namespace le
{
void assertMsg(bool predicate, std::string_view message, std::string_view fileName, u64 lineNumber);
void debugBreak();
} // namespace le
