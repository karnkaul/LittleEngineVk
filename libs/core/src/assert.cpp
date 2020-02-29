#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#if _WIN64
#include <Windows.h>
#elif __linux__
#include <cstring>
#include <fcntl.h> 
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "core/assert.hpp"
#include "core/log.hpp"

namespace le
{
namespace
{
bool IsDebuggerAttached()
{
	bool ret = false;
#if _WIN64
	ret = IsDebuggerPresent() != 0;
#elif __linux__
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
} // namespace

void debugBreak()
{
#if _MSC_VER
	__debugbreak();
#elif __linux__ || __MINGW32__
#ifdef SIGTRAP
	raise(SIGTRAP);
#endif
#endif
	return;
}

void assertMsg(bool predicate, std::string_view message, std::string_view fileName, u64 lineNumber)
{
	if (predicate)
	{
		return;
	}
	std::cerr << "Assertion failed: " << message << " | " << fileName << " (" << lineNumber << ")" << std::endl;
	LOG_E("Assertion failed: {} | {} ({})", message, fileName, lineNumber);
	if (IsDebuggerAttached())
	{
#if _MSC_VER
		OutputDebugStringA(message.data());
#endif
		debugBreak();
	}
	else
	{
#if defined(LEVK_DEBUG)
		debugBreak();
#else
		assert(false && message.data());
#endif
	}
	return;
}
} // namespace le
