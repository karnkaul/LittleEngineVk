#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/os.hpp>
#if defined(LEVK_RUNTIME_MSVC)
#include <Windows.h>
#endif

namespace le
{
void assertMsg(bool predicate, std::string_view message, std::string_view fileName, u64 lineNumber)
{
	if (predicate)
	{
		return;
	}
	std::cerr << "Assertion failed: " << message << " | " << fileName << " (" << lineNumber << ")" << std::endl;
	LOG_E("Assertion failed: {} | {} ({})", message, fileName, lineNumber);
	if (os::isDebuggerAttached())
	{
#if defined(LEVK_OS_WINX)
		OutputDebugStringA(message.data());
#endif
		os::debugBreak();
	}
	else
	{
#if defined(LEVK_DEBUG)
		os::debugBreak();
#else
		assert(false && message.data());
#endif
	}
	return;
}
} // namespace le
