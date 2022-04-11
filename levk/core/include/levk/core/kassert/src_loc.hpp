#pragma once
#define LE_MAKE_SRC_LOC()                                                                                                                                      \
	::le::SrcLoc { __FILE__, __func__, __LINE__ }

namespace le {
struct SrcLoc {
	char const* file = "(Unknown)";
	char const* function = "(Unknown)";
	int line_number = 0;

	constexpr char const* file_name() const { return file; }
	constexpr char const* function_name() const { return function; }
	constexpr int line() const { return line_number; }
};
} // namespace le
