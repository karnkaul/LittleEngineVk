#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <fmt/format.h>
#include "std_types.hpp"

#if (defined(_WIN32) || defined(_WIN64))
#define LEVK_OS_WINX
#if defined(__arm__)
#define LEVK_ARCH_ARM64
#elif !defined(_WIN64)
#define LEVK_ARCH_X86
#else
#define LEVK_ARCH_x64
#endif
#elif defined(__linux__)
#define LEVK_OS_LINUX
#if defined(__ANDROID__)
#define LEVK_OS_ANDROID
#endif
#if defined(__arm__)
#define LEVK_ARCH_ARM64
#elif defined(__x86_64__)
#define LEVK_ARCH_X64
#elif defined(__i386__)
#define LEVK_ARCH_X86
#else
#define LEVK_ARCH_UNSUPPORTED
#endif
#else
#define LEVK_OS_UNSUPPORTED
#endif

#if defined(_MSC_VER)
#define LEVK_RUNTIME_MSVC
#elif (defined(__GNUG__) || defined(__clang__))
#define LEVK_RUNTIME_LIBSTDCXX
#else
#define LEVK_RUNTIME_UNKNOWN
#endif

#if defined(__clang__)
#define LEVK_COMPILER_CLANG
#elif defined(__GNUG__)
#define LEVK_COMPILER_GCC
#elif defined(_MSC_VER)
#define LEVK_COMPILER_VCXX
#else
#define LEVK_COMPILER_UNKNOWN
#endif

namespace le
{
namespace os
{
enum class Dir : s8
{
	eWorking,
	eExecutable,
	eCOUNT_
};

struct Args
{
	s32 argc = 0;
	char* const* const argv = nullptr;
};

struct Service final
{
	Service(Args const& args);
	~Service();
};

void init(Args const& args);
std::string argv0();
std::filesystem::path dirPath(Dir dir);
std::vector<std::string_view> const& args();
bool isDefined(std::string_view arg);

bool isDebuggerAttached();
void debugBreak();

bool sysCall(std::string_view command);

template <typename Arg1, typename... Args>
bool sysCall(std::string_view expr, Arg1 arg1, Args... args)
{
	auto const command = fmt::format(expr, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
	return sysCall(command);
}
} // namespace os
} // namespace le
