#pragma once
#include <le/build_version.hpp>

namespace le {
constexpr bool debug_v =
#if defined(LE_DEBUG)
	true;
#else
	false;
#endif
} // namespace le
