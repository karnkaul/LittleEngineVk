#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <fmt/format.h>
#include "log_config.hpp"
#include "os.hpp"
#include "std_types.hpp"
#include "time.hpp"
#if defined(LEVK_DEBUG)
#include <stdexcept>
#include "assert.hpp"
#endif

#if defined(LEVK_DEBUG)
/**
 * Variable     : LEVK_LOG_DEBUG
 * Description  : Enables LOG_D and LOGIF_D macros (Level::eDebug)
 */
#if !defined(LEVK_LOG_DEBUG)
#define LEVK_LOG_DEBUG
#endif

/**
 * Variable     : LEVK_LOG_CATCH_FMT_EXCEPTIONS
 * Description  : Encloses fmt::format(...) in a try-catch block and calls ASSERT on a runtime exception
 */
#if !defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
#define LEVK_LOG_CATCH_FMT_EXCEPTIONS
#endif
#endif

#define LOG(level, text, ...) le::log::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGIF(predicate, level, text, ...)                               \
	if (predicate)                                                       \
	{                                                                    \
		le::log::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__); \
	}
#define LOG_E(text, ...) LOG(le::log::Level::eError, text, ##__VA_ARGS__)
#define LOGIF_E(predicate, text, ...) LOGIF(predicate, le::log::Level::eError, text, ##__VA_ARGS__)
#define LOG_W(text, ...) LOG(le::log::Level::eWarning, text, ##__VA_ARGS__)
#define LOGIF_W(predicate, text, ...) LOGIF(predicate, le::log::Level::eWarning, text, ##__VA_ARGS__)
#define LOG_I(text, ...) LOG(le::log::Level::eInfo, text, ##__VA_ARGS__)
#define LOGIF_I(predicate, text, ...) LOGIF(predicate, le::log::Level::eInfo, text, ##__VA_ARGS__)

#if defined(LEVK_LOG_DEBUG)
#define LOG_D(text, ...) LOG(le::log::Level::eDebug, text, ##__VA_ARGS__)
#define LOGIF_D(predicate, text, ...) LOGIF(predicate, le::log::Level::eDebug, text, ##__VA_ARGS__)
#else
#define LOG_D(text, ...)
#define LOGIF_D(predicate, text, ...)
#endif

namespace le::log
{
struct Service final
{
	Service(std::filesystem::path const& path, Time pollRate = 500ms);
	~Service();
};

void logText(Level level, std::string text, std::string_view file, u64 line);

template <typename... Args>
void fmtLog(Level level, std::string_view text, std::string_view file, u64 line, Args... args)
{
	if ((u8)level >= (u8)g_minLevel)
	{
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
		try
#endif
		{
			logText(level, fmt::format(text, args...), file, line);
		}
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
		catch (std::exception const& e)
		{
			ASSERT(false, e.what());
		}
#endif
	}
}

void logToFile(std::filesystem::path path, Time pollRate = 500ms);
void stopFileLogging();
} // namespace le::log
