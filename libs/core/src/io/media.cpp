#include <core/io/media.hpp>
#include <core/log.hpp>
#include <core/utils/string.hpp>

namespace le::io {
std::optional<std::string> Media::string(Path const& uri) const {
	if (auto str = sstream(uri)) { return str->str(); }
	return std::nullopt;
}

bool Media::present(Path const& uri, std::optional<LogLevel> absent) const {
	if (!findPrefixed(uri)) {
		if (absent) { log(*absent, "[{}] [{}] not found in {}!", utils::tName(this), uri.generic_string(), info().name); }
		return false;
	}
	return true;
}
} // namespace le::io
