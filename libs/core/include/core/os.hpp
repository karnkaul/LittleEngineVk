#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "core/std_types.hpp"

#if (defined(_WIN32) || defined(_WIN64))
#define LE3D_OS_WINX
#if defined(__arm__)
#define LE3D_ARCH_ARM64
#elif !defined(_WIN64)
#define LE3D_ARCH_X86
#else
#define LE3D_ARCH_x64
#endif
#elif defined(__linux__)
#if defined(__ANDROID__)
#define LE3D_OS_ANDROID
#else
#define LE3D_OS_LINUX
#endif
#if defined(__arm__)
#define LE3D_ARCH_ARM64
#elif defined(__x86_64__)
#define LE3D_ARCH_X64
#elif defined(__i386__)
#define LE3D_ARCH_X86
#else
#define LE3D_ARCH_UNSUPPORTED
#endif
#else
#define LE3D_OS_UNSUPPORTED
#endif

#if defined(_MSC_VER)
#define LE3D_RUNTIME_MSVC
#elif (defined(__GNUG__) || defined(__clang__))
#define LE3D_RUNTIME_LIBSTDCXX
#else
#define LE3D_RUNTIME_UNKNOWN
#endif

namespace le
{
namespace os
{
inline std::string_view g_EOL = "\n";

enum class Dir
{
	Working,
	Executable
};

struct Args
{
	s32 argc = 0;
	char* const* const argv = nullptr;
};

void init(Args const& args);
std::string_view argv0();
std::filesystem::path dirPath(Dir dir);
std::vector<std::string_view> const& args();
bool isDefined(std::string_view arg);
} // namespace os
} // namespace le
