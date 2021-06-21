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

namespace le {
void ensure(bool pred, std::string_view msg, char const* fl, char const* fn, int ln) {
	if constexpr (levk_ensures) {
		if (!pred) {
			auto const m = fmt::format("Ensure failed: {}\n\t{}:{} [{}]", msg, fl, ln, fn);
			std::cerr << m << std::endl;
#if defined(LEVK_OS_WINDOWS)
			OutputDebugStringA(msg.data());
#endif
			os::debugBreak();
		}
	}
}
} // namespace le
