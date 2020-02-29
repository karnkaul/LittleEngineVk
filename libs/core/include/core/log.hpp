#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <fmt/format.h>
#include "core/std_types.hpp"
#include "core/time.hpp"

#if defined(LEVK_DEBUG)
/**
 * Variable     : LEVK_DEBUG_LOG
 * Description  : Used to enable LOG_D and LOGIF_D macros (Level::Debug)
 */
#if !defined(LEVK_DEBUG_LOG)
#define LEVK_DEBUG_LOG
#endif
/**
 * Variable     : LEVK_LOG_SOURCE_LOCATION
 * Description  : Used to append source file and line number to log output
 */
#if !defined(LEVK_LOG_SOURCE_LOCATION)
#define LEVK_LOG_SOURCE_LOCATION
#endif
#endif

#define LOG(level, text, ...) le::log::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGIF(predicate, level, text, ...)                               \
	if (predicate)                                                       \
	{                                                                    \
		le::log::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__); \
	}
#define LOG_E(text, ...) LOG(le::log::Level::Error, text, ##__VA_ARGS__)
#define LOGIF_E(predicate, text, ...) LOGIF(predicate, le::log::Level::Error, text, ##__VA_ARGS__)
#define LOG_W(text, ...) LOG(le::log::Level::Warning, text, ##__VA_ARGS__)
#define LOGIF_W(predicate, text, ...) LOGIF(predicate, le::log::Level::Warning, text, ##__VA_ARGS__)
#define LOG_I(text, ...) LOG(le::log::Level::Info, text, ##__VA_ARGS__)
#define LOGIF_I(predicate, text, ...) LOGIF(predicate, le::log::Level::Info, text, ##__VA_ARGS__)

#if defined(LEVK_DEBUG_LOG)
#define LOG_D(text, ...) LOG(le::log::Level::Debug, text, ##__VA_ARGS__)
#define LOGIF_D(predicate, text, ...) LOGIF(predicate, le::log::Level::Debug, text, ##__VA_ARGS__)
#else
#define LOG_D(text, ...)
#define LOGIF_D(predicate, text, ...)
#endif

namespace le::log
{
enum class Level : u8
{
	Debug = 0,
	Info,
	Warning,
	Error,
	COUNT_
};

struct HFileLogger final
{
	~HFileLogger();
};

void logText(Level level, std::string text, std::string_view file, u64 line);

template <typename... Args>
void fmtLog(Level level, std::string_view text, std::string_view file, u64 line, Args... args)
{
	logText(level, fmt::format(text, args...), file, line);
}

[[nodiscard]] std::unique_ptr<HFileLogger> logToFile(std::filesystem::path path, Time pollRate = Time::from_s(0.5f));
} // namespace le::log
