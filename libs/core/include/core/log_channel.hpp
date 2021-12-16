#pragma once
#include <core/log.hpp>

namespace le {
enum LogChannelMask : LogChannel {
	LC_EndUser = 1 << 0,
	LC_LibUser = 1 << 1,
	LC_Library = 1 << 2,
};

constexpr LogChannel log_channels_v = levk_pre_release ? LC_EndUser | LC_Library | LC_LibUser : LC_EndUser;
} // namespace le
