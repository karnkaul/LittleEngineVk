#pragma once
#include <core/std_types.hpp>
#include <dumb_log/dumb_log.hpp>

constexpr bool levk_log_debug = levk_debug;

namespace le {
using LogLevel = dlog::level;
using LogChannel = dlog::channel;

template <typename... Args>
void logD(std::string_view fmt, Args const&... args);
template <typename... Args>
void logI(std::string_view fmt, Args const&... args);
template <typename... Args>
void logW(std::string_view fmt, Args const&... args);
template <typename... Args>
void logE(std::string_view fmt, Args const&... args);

template <typename... Args>
void logD(LogChannel ch, std::string_view fmt, Args const&... args);
template <typename... Args>
void logI(LogChannel ch, std::string_view fmt, Args const&... args);
template <typename... Args>
void logW(LogChannel ch, std::string_view fmt, Args const&... args);
template <typename... Args>
void logE(LogChannel ch, std::string_view fmt, Args const&... args);

template <typename Pred, typename... Args>
void log_if(Pred pred, LogLevel level, std::string_view fmt, Args const&... args);
template <typename Pred, typename... Args>
void logD_if([[maybe_unused]] Pred pred, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args const&... args);
template <typename Pred, typename... Args>
void logI_if(Pred pred, std::string_view fmt, Args const&... args);
template <typename Pred, typename... Args>
void logW_if(Pred pred, std::string_view fmt, Args const&... args);
template <typename Pred, typename... Args>
void logE_if(Pred pred, std::string_view fmt, Args const&... args);
} // namespace le

template <typename... Args>
void le::logD([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args const&... args) {
	if constexpr (levk_log_debug) { dlog::log(LogLevel::debug, fmt, args...); }
}
template <typename... Args>
void le::logI(std::string_view fmt, Args const&... args) {
	dlog::log(LogLevel::info, fmt, args...);
}
template <typename... Args>
void le::logW(std::string_view fmt, Args const&... args) {
	dlog::log(LogLevel::warn, fmt, args...);
}
template <typename... Args>
void le::logE(std::string_view fmt, Args const&... args) {
	dlog::log(LogLevel::error, fmt, args...);
}

template <typename... Args>
void le::logD([[maybe_unused]] LogChannel ch, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args const&... args) {
	if constexpr (levk_log_debug) { dlog::log(LogLevel::debug, ch, fmt, args...); }
}
template <typename... Args>
void le::logI(LogChannel ch, std::string_view fmt, Args const&... args) {
	dlog::log(LogLevel::info, ch, fmt, args...);
}
template <typename... Args>
void le::logW(LogChannel ch, std::string_view fmt, Args const&... args) {
	dlog::log(LogLevel::warn, ch, fmt, args...);
}
template <typename... Args>
void le::logE(LogChannel ch, std::string_view fmt, Args const&... args) {
	dlog::log(LogLevel::error, ch, fmt, args...);
}

template <typename Pred, typename... Args>
void le::log_if(Pred pred, LogLevel level, std::string_view fmt, Args const&... args) {
	if (pred) { dlog::log(level, fmt, args...); }
}
template <typename Pred, typename... Args>
void le::logD_if([[maybe_unused]] Pred pred, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args const&... args) {
	if constexpr (levk_log_debug) { log_if(pred, LogLevel::debug, fmt, args...); }
}
template <typename Pred, typename... Args>
void le::logI_if(Pred pred, std::string_view fmt, Args const&... args) {
	log_if(pred, LogLevel::info, fmt, args...);
}
template <typename Pred, typename... Args>
void le::logW_if(Pred pred, std::string_view fmt, Args const&... args) {
	log_if(pred, LogLevel::warn, fmt, args...);
}
template <typename Pred, typename... Args>
void le::logE_if(Pred pred, std::string_view fmt, Args const&... args) {
	log_if(pred, LogLevel::error, fmt, args...);
}
