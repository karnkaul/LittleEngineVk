#pragma once
#include <optional>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <core/erased_ptr.hpp>
#include <core/io/path.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>

namespace le::os {
enum class OS : s8 { eWindows, eLinux, eAndroid, eUnknown };
enum class Arch : s8 { eX64, eARM64, eX86, eUnknown };
enum class StdLib : s8 { eMSVC, eLibStdCXX, eUnknown };
enum class Compiler : s8 { eClang, eGCC, eVCXX, eUnknown };
} // namespace le::os

#if (defined(_WIN32) || defined(_WIN64))
#define LEVK_OS_WINDOWS
inline constexpr le::os::OS levk_OS = le::os::OS::eWindows;
inline constexpr std::string_view levk_OS_name = "Windows";
#if defined(__arm__)
#define LEVK_ARCH_ARM64
inline constexpr le::os::Arch levk_arch = le::os::Arch::eARM64;
inline constexpr std::string_view levk_arch_name = "ARM64";
#elif !defined(_WIN64)
#define LEVK_ARCH_X86
inline constexpr le::os::Arch levk_arch = le::os::Arch::eX86;
inline constexpr std::string_view levk_arch_name = "x86";
#else
#define LEVK_ARCH_x64
inline constexpr le::os::Arch levk_arch = le::os::Arch::eX64;
inline constexpr std::string_view levk_arch_name = "x64";
#endif
#elif defined(__linux__)
#if defined(__ANDROID__)
#define LEVK_OS_ANDROID
inline constexpr le::os::OS levk_OS = le::os::OS::eAndroid;
inline constexpr std::string_view levk_OS_name = "Android";
#else
#define LEVK_OS_LINUX
inline constexpr le::os::OS levk_OS = le::os::OS::eLinux;
inline constexpr std::string_view levk_OS_name = "Linux";
#endif
#if defined(__arm__)
#define LEVK_ARCH_ARM64
inline constexpr le::os::Arch levk_arch = le::os::Arch::eARM64;
inline constexpr std::string_view levk_arch_name = "ARM64";
#elif defined(__x86_64__)
#define LEVK_ARCH_X64
inline constexpr le::os::Arch levk_arch = le::os::Arch::eX64;
inline constexpr std::string_view levk_arch_name = "x64";
#elif defined(__i386__)
#define LEVK_ARCH_X86
inline constexpr le::os::Arch levk_arch = le::os::Arch::eX86;
inline constexpr std::string_view levk_arch_name = "x86";
#else
#define LEVK_ARCH_UNSUPPORTED
inline constexpr le::os::Arch levk_arch = le::os::Arch::eUnknown;
inline constexpr std::string_view levk_arch_name = "Unknown";
#endif
#else
#define LEVK_OS_UNSUPPORTED
inline constexpr le::os::OS levk_OS = le::os::OS::eUnknown;
inline constexpr std::string_view levk_OS_name = "Unknown";
#endif

#if defined(_MSC_VER)
#define LEVK_RUNTIME_MSVC
inline constexpr le::os::StdLib levk_stdlib = le::os::StdLib::eMSVC;
inline constexpr std::string_view levk_stdlib_name = "MSVC";
#elif (defined(__GNUG__) || defined(__clang__))
#define LEVK_RUNTIME_LIBSTDCXX
inline constexpr le::os::StdLib levk_stdlib = le::os::StdLib::eLibStdCXX;
inline constexpr std::string_view levk_stdlib_name = "libstdc++";
#else
#define LEVK_RUNTIME_UNKNOWN
inline constexpr le::os::StdLib levk_stdlib = le::os::StdLib::eUnknown;
inline constexpr std::string_view levk_stdlib_name = "Unknown";
#endif
inline constexpr bool levk_desktopOS = levk_OS == le::os::OS::eWindows || levk_OS == le::os::OS::eLinux;

#if defined(__clang__)
#define LEVK_COMPILER_CLANG
inline constexpr le::os::Compiler levk_compiler = le::os::Compiler::eClang;
inline constexpr std::string_view levk_compiler_name = "Clang";
#elif defined(__GNUG__)
#define LEVK_COMPILER_GCC
inline constexpr le::os::Compiler levk_compiler = le::os::Compiler::eGCC;
inline constexpr std::string_view levk_compiler_name = "GCC";
#elif defined(_MSC_VER)
#define LEVK_COMPILER_VCXX
inline constexpr le::os::Compiler levk_compiler = le::os::Compiler::eVCXX;
inline constexpr std::string_view levk_compiler_name = "VC++";
#else
#define LEVK_COMPILER_UNKNOWN
inline constexpr le::os::Compiler levk_compiler = le::os::Compiler::eUnknown;
inline constexpr std::string_view levk_compiler_name = "Unknown";
#endif

namespace le {
namespace os {
enum class Dir : s8 { eWorking, eExecutable };

using Args = Span<char const* const>;

///
/// \brief Initialise OS service
///
void args(Args args);
///
/// \brief Obtain `argv[0]`
///
std::string argv0();
///
/// \brief Obtain `argv[0]`
///
std::string exeName();
///
/// \brief Obtain all command line arguments passed to the runtime
///
Args args() noexcept;
///
/// \brief Obtain working/executable directory
///
io::Path dirPath(Dir dir);
///
/// \brief Obtain internal/external storage path
///
io::Path androidStorage(ErasedPtr androidApp, bool bExternal);

///
/// \brief Check if a debugger is attached to the runtime
///
bool debugging();
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
bool sysCall(std::string_view expr, Arg1&& arg1, Args&&... args) {
	auto const command = fmt::format(expr, std::forward<Arg1>(arg1), std::forward<Args>(args)...);
	return sysCall(command);
}
} // namespace os
} // namespace le
