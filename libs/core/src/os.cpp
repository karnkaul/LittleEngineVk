#include <algorithm>
#include <cstdlib>
#include <thread>
#include <core/ensure.hpp>
#include <core/io/reader.hpp>
#include <core/log.hpp>
#include <core/os.hpp>

#if defined(LEVK_OS_WINDOWS)
#include <Windows.h>
#elif defined(LEVK_OS_LINUX) || defined(LEVK_OS_ANDROID)
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(LEVK_OS_ANDROID)
#include <android_native_app_glue.h>
#endif
#endif

namespace le {
namespace {
io::Path g_exeLocation;
io::Path g_exePath;
io::Path g_workingDir;
std::string g_exePathStr;
std::deque<os::ArgsParser::entry> g_args;
} // namespace

void os::args(Args const& args) {
	g_workingDir = io::absolute(io::current_path());
	if (args.argc > 0) {
		ArgsParser parser;
		g_args = parser.parse(args.argc, args.argv);
		auto& arg0 = g_args.front();
		g_exeLocation = io::absolute(arg0.k);
		g_exePath = g_exeLocation.parent_path();
		while (g_exePath.filename().generic_string() == ".") {
			g_exePath = g_exePath.parent_path();
		}
		g_exeLocation = g_exePath / g_exeLocation.filename();
		g_args.pop_front();
	}
}

std::string os::argv0() {
	return g_exeLocation.generic_string();
}

io::Path os::dirPath(Dir dir) {
	switch (dir) {
	default:
	case os::Dir::eWorking:
		if (g_workingDir.empty()) {
			g_workingDir = io::absolute(io::current_path());
		}
		return g_workingDir;
	case os::Dir::eExecutable:
		if (g_exePath.empty()) {
			logW("[OS] Unknown executable path! Using working directory instead [{}]", g_workingDir.generic_string());
			g_exePath = dirPath(Dir::eWorking);
		}
		return g_exePath;
	}
}

io::Path os::androidStorage([[maybe_unused]] ErasedRef const& androidApp, [[maybe_unused]] bool bExternal) {
#if defined(LEVK_OS_ANDROID)
	if (androidApp.contains<android_app*>()) {
		if (android_app* pApp = androidApp.get<android_app*>(); pApp->activity) {
			return bExternal ? pApp->activity->externalDataPath : pApp->activity->internalDataPath;
		}
	}
#endif
	return io::Path();
}

kt::result<io::Path, std::string> os::findData(io::Path pattern, Dir start, u8 maxHeight) {
	auto const root = os::dirPath(start);
	auto data = io::FileReader::findUpwards(root, pattern, maxHeight);
	if (!data) {
		return fmt::format("[OS] {} not found (searched {} levels up from {})", pattern.generic_string(), maxHeight, root.generic_string());
	}
	return data.move();
}

std::deque<os::ArgsParser::entry> const& os::args() noexcept {
	return g_args;
}

bool os::halt(View<Ref<ICmdArg>> cmdArgs) {
	for (auto const& entry : g_args) {
		for (ICmdArg& cmdArg : cmdArgs) {
			auto const matches = cmdArg.keyVariants();
			bool const bMatch = std::any_of(matches.begin(), matches.end(), [&entry](std::string_view m) { return entry.k == m; });
			if (bMatch && cmdArg.halt(entry.v)) {
				return true;
			}
		}
	}
	return false;
}

bool os::debugging() {
	bool ret = false;
#if defined(LEVK_OS_WINDOWS)
	ret = IsDebuggerPresent() != 0;
#elif defined(LEVK_RUNTIME_LIBSTDCXX)
	// search /proc/self/status for "TracerPid: <pid>..."
	// return true if found and <pid> > 0
	static constexpr std::string_view tracerPid = "TracerPid:";
	std::array<char, 4096> buf;
	if (auto f = std::ifstream("/proc/self/status", std::ios::in)) {
		f.read(buf.data(), buf.size());
		std::string_view str(buf.data());
		if (std::size_t const tracer = str.find(tracerPid); tracer < str.size()) {
			std::size_t begin = tracer + tracerPid.size();
			while (begin < str.size() && std::isspace(static_cast<unsigned char>(str[begin]))) {
				++begin;
			}
			std::size_t end = begin;
			while (end < str.size() && !std::isspace(static_cast<unsigned char>(str[end]))) {
				++end;
			}
			if (str = str.substr(begin, end - begin); !str.empty()) {
				if (str.size() == 1) {
					ret = str[0] != '0';
				} else {
					ret = std::all_of(str.begin(), str.end(), [](char const c) { return std::isdigit(static_cast<unsigned char>(c)); });
				}
			}
		}
	}
#endif
	return ret;
}

void os::debugBreak() {
#if defined(LEVK_RUNTIME_MSVC)
	__debugbreak();
#elif defined(LEVK_RUNTIME_LIBSTDCXX)
#ifdef SIGTRAP
	raise(SIGTRAP);
#endif
#endif
	return;
}

bool os::sysCall(std::string_view command) {
	if (std::system(command.data()) == 0) {
		return true;
	}
	return false;
}
} // namespace le
