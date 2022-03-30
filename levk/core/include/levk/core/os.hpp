#pragma once
#include <fmt/format.h>
#include <levk/core/erased_ptr.hpp>
#include <levk/core/io/path.hpp>
#include <levk/core/span.hpp>
#include <levk/core/std_types.hpp>
#include <optional>
#include <string>
#include <vector>

namespace le::os {
enum class OS : s8 { eWindows, eLinux, eApple, eUnknown };
enum class Arch : s8 { eX64, eARM64, eX86, eARM32, eUnknown };
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
#define LEVK_OS_LINUX
inline constexpr le::os::OS levk_OS = le::os::OS::eLinux;
inline constexpr std::string_view levk_OS_name = "Linux";
#if defined(__arm__) || defined(__ARM_ARCH)
#if defined(__ARM_ARCH_ISA_A64)
#define LEVK_ARCH_ARM64
inline constexpr le::os::Arch levk_arch = le::os::Arch::eARM64;
inline constexpr std::string_view levk_arch_name = "ARM64";
#else
#define LEVK_ARCH_ARM32
inline constexpr le::os::Arch levk_arch = le::os::Arch::eARM32;
inline constexpr std::string_view levk_arch_name = "ARM32";
#endif
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
#elif defined(__APPLE__)
#define LEVK_OS_APPLE
inline constexpr le::os::OS levk_OS = le::os::OS::eApple;
inline constexpr std::string_view levk_OS_name = "Apple";
#if defined(__arm__) || defined(__ARM_ARCH)
#if defined(__ARM_ARCH_ISA_A64)
#define LEVK_ARCH_ARM64
inline constexpr le::os::Arch levk_arch = le::os::Arch::eARM64;
inline constexpr std::string_view levk_arch_name = "ARM64";
#else
#define LEVK_ARCH_ARM32
inline constexpr le::os::Arch levk_arch = le::os::Arch::eARM32;
inline constexpr std::string_view levk_arch_name = "ARM32";
#endif
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
///
/// \brief Command line arguments
///
using Args = Span<char const* const>;

///
/// \brief Working environment
///
struct Environment {
	struct Paths {
		io::Path exe;
		io::Path pwd;

		io::Path bin() const { return exe.parent_path(); }
	};

	Paths paths;
	Args args;
	std::string_view arg0;
};

///
/// \brief Initialise environment
///
void environment(Args args);
///
/// \brief Obtain the current environment
///
Environment const& environment() noexcept;

///
/// \brief Check if a debugger is attached to the runtime
///
bool debugging();
///
/// \brief Raise  a breakpoint signal
///
void debugBreak();

///
/// \brief Obtain the CPU ID (if possible)
///
std::string_view cpuID();
} // namespace os
} // namespace le
