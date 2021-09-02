#include <core/log.hpp>
#include <core/os.hpp>
#include <core/utils/error.hpp>

namespace le {
namespace {
std::size_t pathSep(std::string_view path) noexcept {
	auto i = path.find_last_of('/');
	return i < path.size() ? i : path.find_last_of('\\');
}

std::string_view shortPath(std::string_view path) noexcept {
	static constexpr std::string_view dir = "LittleEngineVk";
	if (auto i = path.find(dir); i < path.size()) { return path.substr(i + dir.size() + 1); }
	if (auto i = pathSep(path); i < path.size()) {
		auto const parent = path.substr(0, i);
		if (auto j = pathSep(parent)) { return path.substr(j + 1); }
		return path.substr(i + 1);
	}
	return path;
}
} // namespace

void utils::error(std::string msg, char const* fl, char const* fn, int ln) {
	auto const sp = shortPath(fl);
	logE("Error{} {}\n\t{}:{} [{}]", msg.empty() ? "" : ":", msg, sp, ln, fn);
	if (os::debugging()) { os::debugBreak(); }
	if (g_onError) { (*g_onError)(msg, {fn, sp, ln}); }
	throw Error(msg);
}
} // namespace le
