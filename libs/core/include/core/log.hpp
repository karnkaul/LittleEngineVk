#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <fmt/format.h>
#include <core/assert.hpp>
#include <core/log_config.hpp>
#include <core/os.hpp>
#include <core/std_types.hpp>

#if defined(LEVK_DEBUG)
/**
 * Variable     : LEVK_LOG_DEBUG
 * Description  : Enables LOG_D and LOGIF_D macros (Level::eDebug)
 */
#if !defined(LEVK_LOG_DEBUG)
#define LEVK_LOG_DEBUG
#endif
#endif

#if defined(LEVK_LOG_DEBUG)
constexpr bool levk_logDebug = true;
constexpr bool levk_logCatchFmtExceptions = true;
#else
constexpr bool levk_logDebug = false;
constexpr bool levk_logCatchFmtExceptions = false;
#endif

#define LOG(level, text, ...) ::le::io::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGIF(predicate, level, text, ...)                                                                                                                     \
	do                                                                                                                                                         \
		if (predicate) {                                                                                                                                       \
			::le::io::fmtLog(level, text, __FILE__, __LINE__, ##__VA_ARGS__);                                                                                  \
		}                                                                                                                                                      \
	while (0)
#define LOG_E(text, ...) LOG(::le::io::Level::eError, text, ##__VA_ARGS__)
#define LOGIF_E(predicate, text, ...) LOGIF(predicate, ::le::io::Level::eError, text, ##__VA_ARGS__)
#define LOG_W(text, ...) LOG(::le::io::Level::eWarning, text, ##__VA_ARGS__)
#define LOGIF_W(predicate, text, ...) LOGIF(predicate, ::le::io::Level::eWarning, text, ##__VA_ARGS__)
#define LOG_I(text, ...) LOG(::le::io::Level::eInfo, text, ##__VA_ARGS__)
#define LOGIF_I(predicate, text, ...) LOGIF(predicate, ::le::io::Level::eInfo, text, ##__VA_ARGS__)

#if defined(LEVK_LOG_DEBUG)
#define LOG_D(text, ...) LOG(::le::io::Level::eDebug, text, ##__VA_ARGS__)
#define LOGIF_D(predicate, text, ...) LOGIF(predicate, ::le::io::Level::eDebug, text, ##__VA_ARGS__)
#else
#define LOG_D(text, ...)
#define LOGIF_D(predicate, text, ...)
#endif

namespace le {
namespace stdfs = std::filesystem;
}

namespace le::io {
///
/// \brief RAII wrapper for file logging
///
struct Service final {
	Service(std::optional<stdfs::path> logFilePath);
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
void fmtLog(Level level, std::string_view text, std::string_view file, u64 line, Args&&... args) {
	if ((u8)level >= (u8)g_minLevel) {
		if constexpr (levk_logCatchFmtExceptions) {
			try {
				log(level, fmt::format(text, std::forward<Args>(args)...), file, line);
			} catch (std::exception const& e) {
				ASSERT(false, e.what());
			}
		} else {
			log(level, fmt::format(text, std::forward<Args>(args)...), file, line);
		}
	}
}
} // namespace le::io
