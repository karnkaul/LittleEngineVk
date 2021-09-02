#include <core/ensure.hpp>
#include <core/utils/error.hpp>

namespace le {
void ensure(bool pred, std::string_view msg, char const* fl, char const* fn, int ln) {
	if (!pred) { utils::error(std::string(msg), fl, fn, ln); }
}
} // namespace le
