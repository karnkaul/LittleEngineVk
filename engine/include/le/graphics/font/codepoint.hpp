#pragma once
#include <cstdint>

namespace le::graphics {
enum struct Codepoint : std::uint32_t {
	eTofu = 0u,
	eSpace = 32u,
	eAsciiFirst = eSpace,
	eAsciiLast = 126,
};
} // namespace le::graphics
