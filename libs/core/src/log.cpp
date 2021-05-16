#include <core/log.hpp>
#include <core/os.hpp>

#if defined(LEVK_OS_ANDROID)
#include <android/log.h>

namespace le::detail {
void logAndroid(dl::level level, std::string_view msg, std::string_view tag) {
	int prio = ANDROID_LOG_DEBUG;
	switch (level) {
	case dl::level::error: prio = ANDROID_LOG_ERROR; break;
	case dl::level::warning: prio = ANDROID_LOG_WARN; break;
	case dl::level::info: prio = ANDROID_LOG_INFO; break;
	default: break;
	}
	__android_log_print(prio, tag.data(), "%s", msg.data());
	dl::config::g_on_log(msg, level);
}
} // namespace le::detail
#endif
