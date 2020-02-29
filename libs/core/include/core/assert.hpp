#pragma once

/**
 * Variable     : ASSERTS
 * Description  : Used to log time taken to load meshes, textures, etc from model data
 */
#if defined(LEVK_DEBUG)
#if !defined(LE3D_ASSERTS)
#define LE3D_ASSERTS
#endif
#endif

#if defined(LE3D_ASSERTS)
#define ASSERT(predicate, errorMessage) le::assertMsg(!!(predicate), #errorMessage, __FILE__, __LINE__)
#define ASSERT_STR(predicate, szStr) le::assertMsg(!!(predicate), szStr, __FILE__, __LINE__)
#else
#define ASSERT(disabled, _disabled)
#define ASSERT_STR(disabled, _disabled)
#endif

namespace le
{
void assertMsg(bool expr, char const* message, char const* fileName, long lineNumber);
void debugBreak();
} // namespace le
