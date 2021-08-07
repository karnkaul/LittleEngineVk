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
#endif

namespace le {
namespace {
os::Environment g_env;
} // namespace

void os::environment(Args args) {
	g_env.paths.pwd = io::absolute(io::current_path());
	g_env.args = args;
	if (!g_env.args.empty()) {
		g_env.arg0 = g_env.args[0];
		io::Path const arg0 = io::absolute(g_env.arg0);
		if (!arg0.empty() && io::is_regular_file(arg0)) {
			auto bin = arg0.parent_path();
			while (bin.filename().string() == ".") { bin = bin.parent_path(); }
			g_env.paths.exe = std::move(bin) / arg0.filename();
		}
	}
}

os::Environment const& os::environment() noexcept { return g_env; }

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
			while (begin < str.size() && std::isspace(static_cast<unsigned char>(str[begin]))) { ++begin; }
			std::size_t end = begin;
			while (end < str.size() && !std::isspace(static_cast<unsigned char>(str[end]))) { ++end; }
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
	if (std::system(command.data()) == 0) { return true; }
	return false;
}
} // namespace le
