#pragma once
#include <array>
#include <string>
#include <core/std_types.hpp>

namespace le::io {
#if defined(LEVK_DEBUG)
constexpr bool g_log_bSourceLocation = true;
#else
constexpr bool g_log_bSourceLocation = false;
#endif
enum class Level : s8 { eDebug, eInfo, eWarning, eError, eCOUNT_ };

inline Level g_minLevel =
#if defined(LEVK_DEBUG)
	Level::eDebug;
#else
	Level::eInfo;
#endif

using OnLog = void (*)(std::string_view, Level);
inline OnLog g_onLog = nullptr;

inline constexpr std::array g_logLevels = {"Debug"sv, "Info"sv, "Warning"sv, "Error"sv};
} // namespace le::io
