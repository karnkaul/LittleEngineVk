#include <fmt/chrono.h>
#include <core/time.hpp>

namespace le {
std::string time::format(SysTime const& tm, std::string_view fmt) {
	return fmt::format(fmt.data(), fmt::localtime(stdch::system_clock::to_time_t(tm)));
}
} // namespace le
