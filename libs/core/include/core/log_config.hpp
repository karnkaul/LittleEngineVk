#pragma once
#include "std_types.hpp"
#include <string>

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

using OnLog = void (*)(std::string_view, Level);
inline OnLog g_onLog = nullptr;
} // namespace le::log
