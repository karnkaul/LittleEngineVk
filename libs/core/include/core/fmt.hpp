#pragma once
#include "core/os.hpp"
#if defined(LEVK_OS_WINX) && defined(LEVK_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#include <fmt/format.h>
#if defined(LEVK_OS_WINX) && defined(LEVK_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
