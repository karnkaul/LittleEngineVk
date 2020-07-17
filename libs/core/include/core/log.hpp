#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <fmt/format.h>
#include <core/log_config.hpp>
#include <core/os.hpp>
#include <core/std_types.hpp>
#include <core/time.hpp>
#if defined(LEVK_DEBUG)
#include <stdexcept>
#include <core/assert.hpp>
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

#define LOG(level, text, ...) le::io::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGIF(predicate, level, text, ...)                              \
	if (predicate)                                                      \
	{                                                                   \
		le::io::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__); \
	}
#define LOG_E(text, ...) LOG(le::io::Level::eError, text, ##__VA_ARGS__)
#define LOGIF_E(predicate, text, ...) LOGIF(predicate, le::io::Level::eError, text, ##__VA_ARGS__)
#define LOG_W(text, ...) LOG(le::io::Level::eWarning, text, ##__VA_ARGS__)
#define LOGIF_W(predicate, text, ...) LOGIF(predicate, le::io::Level::eWarning, text, ##__VA_ARGS__)
#define LOG_I(text, ...) LOG(le::io::Level::eInfo, text, ##__VA_ARGS__)
#define LOGIF_I(predicate, text, ...) LOGIF(predicate, le::io::Level::eInfo, text, ##__VA_ARGS__)

#if defined(LEVK_LOG_DEBUG)
#define LOG_D(text, ...) LOG(le::io::Level::eDebug, text, ##__VA_ARGS__)
#define LOGIF_D(predicate, text, ...) LOGIF(predicate, le::io::Level::eDebug, text, ##__VA_ARGS__)
#else
#define LOG_D(text, ...)
#define LOGIF_D(predicate, text, ...)
#endif

namespace le::io
{
///
/// \brief RAII wrapper for file logging
///
struct Service final
{
	Service(std::filesystem::path const& path, Time pollRate = 500ms);
	~Service();
};

///
/// \brief Print to `stdout`
///
void log(Level level, std::string text, std::string_view file, u64 line);

///
/// \brief Print to `stdout`
///
template <typename... Args>
void fmtLog(Level level, std::string_view text, std::string_view file, u64 line, Args&&... args)
{
	if ((u8)level >= (u8)g_minLevel)
	{
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
		try
#endif
		{
			log(level, fmt::format(text, std::forward<Args>(args)...), file, line);
		}
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
		catch (std::exception const& e)
		{
			ASSERT(false, e.what());
		}
#endif
	}
}

///
/// \brief Start file logging (on another thread)
/// \param path fully qualified path to store log file as
/// \param pollRate rate at which file logging thread polls for new logs
///
void logToFile(std::filesystem::path path, Time pollRate = 500ms);
///
/// \brief Stop file logging
///
void stopFileLogging();
} // namespace le::io
