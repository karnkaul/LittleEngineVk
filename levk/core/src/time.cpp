#include <fmt/chrono.h>
#include <levk/core/time.hpp>

namespace le {
std::string time::format(SysTime const& tm, std::string_view fmt) { return fmt::format(fmt::runtime(fmt), fmt::localtime(stdch::system_clock::to_time_t(tm))); }

std::string time::format(Time_s duration, std::string_view fmt) {
	if (fmt.empty()) {
		if (duration < 30s) { return fmt::format("{:%Ss}", duration); }
		if (duration < stdch::minutes(1)) {
			fmt = "{:%Ss}";
		} else if (duration < stdch::hours(1)) {
			fmt = "{:%Mm%Ss}";
		} else {
			fmt = "{:%Hh%Mm%Ss}";
		}
	}
	return fmt::format(fmt::runtime(fmt), time::cast<stdch::seconds>(duration));
}
} // namespace le
