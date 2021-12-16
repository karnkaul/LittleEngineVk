#include <core/log.hpp>
#include <core/utils/src_info.hpp>

namespace le::utils {
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

void SrcInfo::logMsg(char const* pre, char const* msg, dlog::level level) const { log(level, "{}{}\n\t{}:{} [{}]", pre, msg, shortPath(file), line, function); }
} // namespace le::utils
