#pragma once
#include <cstdint>

namespace spaced::graphics {
enum struct Codepoint : std::uint32_t {
	eTofu = 0u,
	eSpace = 32u,
	eAsciiFirst = eSpace,
	eAsciiLast = 126,
};
} // namespace spaced::graphics
