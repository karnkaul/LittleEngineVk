#include <signal.h>
#include <levk/core/log.hpp>
#include <levk/core/maths.hpp>
#include <levk/core/os.hpp>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <thread>

#if defined(LEVK_OS_WINDOWS)
#include <Windows.h>
#elif defined(LEVK_OS_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#elif defined(LEVK_OS_APPLE)
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#endif

namespace le {
namespace {
os::Environment g_env;
std::string g_cpuID;
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
}

std::string_view os::cpuID() {
	if (g_cpuID.empty()) {
		if constexpr (levk_arch == Arch::eX64 || levk_arch == Arch::eX86) {
			u32 regs[4] = {};
#if defined(_MSC_VER)
			__cpuid((int*)regs, 0);
#elif defined(LEVK_ARCH_X64) || defined(LEVK_ARCH_X86)
			asm volatile("cpuid" : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3]) : "a"(0), "c"(0));
#endif
			auto strv = [&regs](std::size_t i) { return std::string_view(reinterpret_cast<char const*>(&regs[i]), 4); };
			std::ostringstream str;
			str << strv(1) << strv(3) << strv(2);
			g_cpuID = str.str();
		} else {
			g_cpuID = "(unknown)";
		}
	}
	return g_cpuID;
}
} // namespace le
