#pragma once
#include "std_types.hpp"

namespace le::log
{
#if defined(LEVK_DEBUG)
constexpr bool g_log_bSourceLocation = true;
#else
constexpr bool g_log_bSourceLocation = false;
#endif
enum class Level : u8
{
	eDebug,
	eInfo,
	eWarning,
	eError,
	eCOUNT_
};

inline Level g_minLevel =
#if defined(LEVK_DEBUG)
	Level::eDebug;
#else
	Level::eInfo;
#endif
} // namespace le::log
