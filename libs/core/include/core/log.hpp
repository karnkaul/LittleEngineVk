#pragma once
#include <core/os.hpp>
#include <dumb_log/log.hpp>

namespace le {
template <typename... Args>
void logD([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args);
template <typename... Args>
void logI(std::string_view fmt, Args&&... args);
template <typename... Args>
void logW(std::string_view fmt, Args&&... args);
template <typename... Args>
void logE(std::string_view fmt, Args&&... args);
template <typename Pred, typename... Args>
void log_if(Pred pred, dl::level level, std::string_view fmt, Args&&... args);
template <typename Pred, typename... Args>
void logD_if([[maybe_unused]] Pred pred, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args);
template <typename Pred, typename... Args>
void logI_if(Pred pred, std::string_view fmt, Args&&... args);
template <typename Pred, typename... Args>
void logW_if(Pred pred, std::string_view fmt, Args&&... args);
template <typename Pred, typename... Args>
void logE_if(Pred pred, std::string_view fmt, Args&&... args);
} // namespace le

// impl
namespace le::detail {
template <typename... Args>
void logImpl(dl::level level, std::string_view fmt, Args&&... args) {
#if defined(LEVK_OS_ANDROID)
	extern void logAndroid(dl::level level, std::string_view msg, std::string_view tag);
	logAndroid(level, dl::format(level, fmt::format(fmt, std::forward<Args>(args)...)), "levk");
#else
	dl::log(level, fmt, std::forward<Args>(args)...);
#endif
}
} // namespace le::detail

template <typename... Args>
void le::logD([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args) {
	if constexpr (dl::dlog_debug) { ::le::detail::logImpl(dl::level::debug, fmt, std::forward<Args>(args)...); }
}
template <typename... Args>
void le::logI(std::string_view fmt, Args&&... args) {
	::le::detail::logImpl(dl::level::info, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void le::logW(std::string_view fmt, Args&&... args) {
	::le::detail::logImpl(dl::level::warning, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void le::logE(std::string_view fmt, Args&&... args) {
	::le::detail::logImpl(dl::level::error, fmt, std::forward<Args>(args)...);
}
template <typename Pred, typename... Args>
void le::log_if(Pred pred, dl::level level, std::string_view fmt, Args&&... args) {
	if (pred) { ::le::detail::logImpl(level, fmt, std::forward<Args>(args)...); }
}
template <typename Pred, typename... Args>
void le::logD_if([[maybe_unused]] Pred pred, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args) {
	if constexpr (dl::dlog_debug) { log_if(pred, dl::level::debug, fmt, std::forward<Args>(args)...); }
}
template <typename Pred, typename... Args>
void le::logI_if(Pred pred, std::string_view fmt, Args&&... args) {
	log_if(pred, dl::level::info, fmt, std::forward<Args>(args)...);
}
template <typename Pred, typename... Args>
void le::logW_if(Pred pred, std::string_view fmt, Args&&... args) {
	log_if(pred, dl::level::warning, fmt, std::forward<Args>(args)...);
}
template <typename Pred, typename... Args>
void le::logE_if(Pred pred, std::string_view fmt, Args&&... args) {
	log_if(pred, dl::level::error, fmt, std::forward<Args>(args)...);
}
