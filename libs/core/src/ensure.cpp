#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <fmt/format.h>
#include <core/ensure.hpp>
#include <core/os.hpp>
#if defined(LEVK_RUNTIME_MSVC)
#include <Windows.h>
#endif

void le::ensureImpl(std::string_view message, src_loc const& sl) {
	auto const msg = fmt::format("Ensure failed: {}\n\t{}:{} [{}]", message, sl.file_name(), sl.line(), sl.function_name());
	std::cerr << msg << std::endl;
#if defined(LEVK_OS_WINX)
	OutputDebugStringA(msg.data());
#endif
	os::debugBreak();
}
