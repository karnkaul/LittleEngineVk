#include <iomanip>
#include <sstream>
#include <fmt/chrono.h>
#include <core/time.hpp>

namespace le {
std::string time::format(SysTime const& tm, std::string_view fmt) { return fmt::format(fmt.data(), fmt::localtime(stdch::system_clock::to_time_t(tm))); }

std::string time::format(Time_s duration) {
	std::stringstream str;
	str << std::setfill('0');
	auto h = cast<stdch::hours>(duration);
	auto const d = h % stdch::hours(24);
	if (d > stdch::hours(0)) {
		h -= d;
		str << d.count() << ':' << std::setw(2);
	}
	if (h > stdch::hours(0)) {
		str << h.count() << ':';
	}
	auto const m = cast<stdch::minutes>(duration) % stdch::minutes(60);
	auto const s = cast<stdch::seconds>(duration) % stdch::seconds(60);
	str << std::setw(2) << m.count() << ':' << std::setw(2) << s.count();
	if (h <= stdch::hours(0)) {
		auto const ms = cast<stdch::milliseconds>(duration) % stdch::milliseconds(1000);
		str << ':' << std::setw(3) << ms.count();
	}
	return str.str();
}
} // namespace le
