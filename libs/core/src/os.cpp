#include <algorithm>
#include <cstdlib>
#include <thread>
#include <core/ensure.hpp>
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
std::vector<std::string_view> g_args;

std::vector<std::string_view> buildArgs(os::Args const& args) {
	std::vector<std::string_view> ret;
	ret.reserve(std::size_t(args.argc));
	for (int i = 0; i < args.argc; ++i) {
		ret.push_back(args.argv[std::size_t(i)]);
	}
	return ret;
}
} // namespace

void os::args(Args const& args) {
	g_workingDir = io::absolute(io::current_path());
	g_args = buildArgs(args);
	if (!g_args.empty()) {
		io::Path const arg0 = io::absolute(g_args.front());
		if (!arg0.empty() && io::is_regular_file(arg0)) {
			g_exePath = arg0.parent_path();
			while (g_exePath.filename().string() == ".") {
				g_exePath = g_exePath.parent_path();
			}
			g_exeLocation = g_exePath / arg0.filename();
		}
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

std::vector<std::string_view> const& os::args() noexcept {
	return g_args;
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
