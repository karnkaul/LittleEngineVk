#include <algorithm>
#include <filesystem>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/gdata.hpp"
#include "core/os.hpp"
#include "threads_impl.hpp"
#if defined(LEVK_OS_WINX)
#include <Windows.h>
#elif defined(LEVK_OS_LINUX)
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

namespace le
{
namespace stdfs = std::filesystem;

namespace
{
stdfs::path g_exeLocation;
stdfs::path g_exePath;
stdfs::path g_workingDir;
std::string g_exePathStr;
std::vector<std::string_view> g_args;
} // namespace

void os::init(Args const& args)
{
#if defined(__linux__)
	s32 threadStatus = XInitThreads();
	if (threadStatus == 0)
	{
		LOG_E("[OS] ERROR calling XInitThreads()! UB follows.");
		threadsImpl::g_maxThreads = 1;
	}
#endif
	g_workingDir = stdfs::absolute(stdfs::current_path());
	if (args.argc > 0)
	{
		g_exeLocation = stdfs::absolute(args.argv[0]);
		g_exePath = g_exeLocation.parent_path();
		while (g_exePath.filename() == ".")
		{
			g_exePath = g_exePath.parent_path();
		}
		for (s32 i = 1; i < args.argc; ++i)
		{
			g_args.push_back(args.argv[i]);
		}
	}
	return;
}

std::string_view os::argv0()
{
	return g_exeLocation.generic_string();
}

stdfs::path os::dirPath(Dir dir)
{
	switch (dir)
	{
	default:
	case os::Dir::Working:
		if (g_workingDir.empty())
		{
			g_workingDir = stdfs::absolute(stdfs::current_path());
		}
		return g_workingDir;
	case os::Dir::Executable:
		if (g_exePath.empty())
		{
			LOG_E("[OS] Unknown executable path! Using working directory instead [{}]", g_workingDir.generic_string());
			g_exePath = dirPath(Dir::Working);
		}
		return g_exePath;
	}
}

std::vector<std::string_view> const& os::args()
{
	return g_args;
}

bool os::isDefined(std::string_view arg)
{
	return std::find_if(g_args.begin(), g_args.end(), [arg](std::string_view s) { return s == arg; }) != g_args.end();
}

bool os::isDebuggerAttached()
{
	bool ret = false;
#if defined(LEVK_OS_WINX)
	ret = IsDebuggerPresent() != 0;
#elif defined(LEVK_OS_LINUX)
	char buf[4096];

	auto const status_fd = ::open("/proc/self/status", O_RDONLY);
	if (status_fd == -1)
	{
		return false;
	}

	auto const num_read = ::read(status_fd, buf, sizeof(buf) - 1);
	if (num_read <= 0)
	{
		return false;
	}

	buf[num_read] = '\0';
	constexpr char tracerPidString[] = "TracerPid:";
	auto const tracer_pid_ptr = ::strstr(buf, tracerPidString);
	if (!tracer_pid_ptr)
	{
		return false;
	}

	for (char const* characterPtr = tracer_pid_ptr + sizeof(tracerPidString) - 1; characterPtr <= buf + num_read; ++characterPtr)
	{
		if (::isspace(*characterPtr))
		{
			continue;
		}
		else
		{
			ret = ::isdigit(*characterPtr) != 0 && *characterPtr != '0';
		}
	}
#endif
	return ret;
}

void os::debugBreak()
{
#if defined(LEVK_RUNTIME_MSVC)
	__debugbreak();
#elif defined(LEVK_RUNTIME_LIBSTDCXX)
#ifdef SIGTRAP
	raise(SIGTRAP);
#endif
#endif
	return;
}
} // namespace le
