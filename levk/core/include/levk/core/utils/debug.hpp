#pragma once
#include <ktl/debug_trap.hpp>
#include <levk/core/os.hpp>

// clang-format off
#if defined(DEBUG_TRAP)
#undef DEBUG_TRAP
#endif
#define DEBUG_TRAP() if (::le::os::debugging()) { KTL_DEBUG_TRAP(); }
// clang-format on
