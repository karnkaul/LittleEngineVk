#pragma once
#include <dumb_log/level.hpp>

namespace le::utils {
///
/// \brief Source information
///
struct SrcInfo {
	char const* function = "(unknown)";
	char const* file = "(unknown)";
	int line = 0;

	void logMsg(char const* pre, char const* msg, dl::level level) const;
};
} // namespace le::utils
