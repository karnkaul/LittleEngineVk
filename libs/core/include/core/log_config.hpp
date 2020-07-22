#pragma once
#include <array>
#include <core/std_types.hpp>
#include <string>

namespace le::io
{
#if defined(LEVK_DEBUG)
constexpr bool g_log_bSourceLocation = true;
#else
constexpr bool g_log_bSourceLocation = false;
#endif
enum class Level : s8
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

inline constexpr std::array<std::string_view, (std::size_t)Level::eCOUNT_> g_logLevels = {"Debug", "Info", "Warning", "Error"};
} // namespace le::io
