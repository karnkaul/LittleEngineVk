#pragma once
#include <core/os.hpp>

// clang-format off
#if defined(DEBUG_TRAP)
#undef DEBUG_TRAP
#endif
#if defined(LEVK_ARCH_X86) || defined(LEVK_ARCH_X64)
#define DEBUG_TRAP() if (::le::os::debugging()) { asm volatile("int3"); }
#else
#define DEBUG_TRAP()
#endif
// clang-format on
