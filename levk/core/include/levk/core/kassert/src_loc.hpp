#pragma once
#if defined(__clang__) || defined(__GNUG__)
#include <experimental/source_location>
#define LE_SRC_LOC std::experimental::source_location;
#else
#include <source_location>
#define LE_SRC_LOC std::source_location;
#endif

namespace le {
using SrcLoc = LE_SRC_LOC;
}

#undef LE_SRC_LOC
