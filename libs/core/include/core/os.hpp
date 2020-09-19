#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <core/std_types.hpp>
#include <kt/args_parser/args_parser.hpp>

namespace le::os
{
enum class OS : s8
{
	eWindows,
	eLinux,
	eAndroid,
	eUnknown
};
enum class Arch : s8
{
	eX64,
	eARM64,
	eUnknown
};
enum class StdLib : s8
{
	eMSVC,
	eLibStdCXX,
	eUnknown
};
enum class Compiler : s8
{
	eClang,
	eGCC,
	eVCXX,
	eUnknown
};
} // namespace le::os

#if (defined(_WIN32) || defined(_WIN64))
#define LEVK_OS_WINX
constexpr le::os::OS levk_OS = le::os::OS::eWindows;
constexpr std::string_view levk_OS_name = "Windows";
#if defined(__arm__)
#define LEVK_ARCH_ARM64
constexpr le::os::Arch levk_arch = le::os::Arch::eARM64;
constexpr std::string_view levk_arch_name = "ARM64";
#elif !defined(_WIN64)
#define LEVK_ARCH_X86
constexpr le::os::Arch levk_arch = le::os::Arch::eX86;
constexpr std::string_view levk_arch_name = "x86";
#else
#define LEVK_ARCH_x64
constexpr le::os::Arch levk_arch = le::os::Arch::eX64;
constexpr std::string_view levk_arch_name = "x64";
#endif
#elif defined(__linux__)
#define LEVK_OS_LINUX
constexpr le::os::OS levk_OS = le::os::OS::eLinux;
constexpr std::string_view levk_OS_name = "Linux";
#if defined(__ANDROID__)
constexpr le::os::OS levk_OS = le::os::OS::eAndroid;
constexpr std::string_view levk_OS_name = "Android";
#define LEVK_OS_ANDROID
#endif
#if defined(__arm__)
#define LEVK_ARCH_ARM64
constexpr le::os::Arch levk_arch = le::os::Arch::eARM64;
constexpr std::string_view levk_arch_name = "ARM64";
#elif defined(__x86_64__)
#define LEVK_ARCH_X64
constexpr le::os::Arch levk_arch = le::os::Arch::eX64;
constexpr std::string_view levk_arch_name = "x64";
#elif defined(__i386__)
#define LEVK_ARCH_X86
constexpr le::os::Arch levk_arch = le::os::Arch::eX86;
constexpr std::string_view levk_arch_name = "x86";
#else
#define LEVK_ARCH_UNSUPPORTED
constexpr le::os::Arch levk_arch = le::os::Arch::eUnknown;
constexpr std::string_view levk_arch_name = "Unknown";
#endif
#else
#define LEVK_OS_UNSUPPORTED
constexpr le::os::OS levk_OS = le::os::OS::eUnknown;
constexpr std::string_view levk_OS_name = "Unknown";
#endif

#if defined(_MSC_VER)
#define LEVK_RUNTIME_MSVC
constexpr le::os::StdLib levk_stdlib = le::os::StdLib::eMSVC;
constexpr std::string_view levk_stdlib_name = "MSVC";
#elif (defined(__GNUG__) || defined(__clang__))
#define LEVK_RUNTIME_LIBSTDCXX
constexpr le::os::StdLib levk_stdlib = le::os::StdLib::eLibStdCXX;
constexpr std::string_view levk_stdlib_name = "libstdc++";
#else
#define LEVK_RUNTIME_UNKNOWN
constexpr le::os::StdLib levk_stdlib = le::os::StdLib::eUnknown;
constexpr std::string_view levk_stdlib_name = "Unknown";
#endif

#if defined(__clang__)
#define LEVK_COMPILER_CLANG
constexpr le::os::Compiler levk_compiler = le::os::Compiler::eClang;
constexpr std::string_view levk_compiler_name = "Clang";
#elif defined(__GNUG__)
#define LEVK_COMPILER_GCC
constexpr le::os::Compiler levk_compiler = le::os::Compiler::eGCC;
constexpr std::string_view levk_compiler_name = "GCC";
#elif defined(_MSC_VER)
#define LEVK_COMPILER_VCXX
constexpr le::os::Compiler levk_compiler = le::os::Compiler::eVCXX;
constexpr std::string_view levk_compiler_name = "VC++";
#else
#define LEVK_COMPILER_UNKNOWN
constexpr le::os::Compiler levk_compiler = le::os::Compiler::eUnknown;
constexpr std::string_view levk_compiler_name = "Unknown";
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
	char const* const* argv = nullptr;
};

using ArgsParser = kt::args_parser<>;

///
/// \brief RAII wrapper for OS service
/// Destructor joins all running threads
///
struct Service final
{
	Service(Args args);
	~Service();
};

///
/// \brief Initialise OS service
///
void args(Args args);
///
/// \brief Obtain `argv[0]`
///
std::string argv0();
///
/// \brief Obtain working/executable directory
///
std::filesystem::path dirPath(Dir dir);
///
/// \brief Obtain all command line arguments passed to the runtime
///
std::deque<ArgsParser::entry> const& args() noexcept;
///
/// \brief Check if a string or its variant was passed as a command line argument
/// \returns `std::nullopt` if arg is not defined, else the value of the arg (empty if key-only arg)
///
template <typename Arg, typename... Args>
std::optional<std::string_view> isDefined(Arg&& key, Args&&... variants) noexcept;

///
/// \brief Check if a debugger is attached to the runtime
///
bool isDebuggerAttached();
///
/// \brief Raise  a breakpoint signal
///
void debugBreak();

///
/// \brief Perform a system call
///
bool sysCall(std::string_view command);

///
/// \brief Perform a system call
///
template <typename Arg1, typename... Args>
bool sysCall(std::string_view expr, Arg1&& arg1, Args&&... args)
{
	auto const command = fmt::format(expr, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
	return sysCall(command);
}

template <typename Arg, typename... Args>
std::optional<std::string_view> isDefined(Arg&& key, Args&&... variants) noexcept
{
	static_assert(std::is_convertible_v<Arg, std::string> && (std::is_convertible_v<Args, std::string> && ...), "Invalid Types!");
	auto const& allArgs = args();
	auto matchAny = [&](ArgsParser::entry const& arg) { return (arg.k == key || ((arg.k == variants) || ...)); };
	auto search = std::find_if(allArgs.begin(), allArgs.end(), matchAny);
	if (search != allArgs.end())
	{
		return search->v;
	}
	return std::nullopt;
}
} // namespace os
} // namespace le
