#pragma once
#include <cstdint>

namespace spaced::graphics {
enum struct TextHeight : std::uint32_t {
	eMin = 10,
	eDefault = 40,
	eMax = 256,
};

constexpr auto clamp(TextHeight in) -> TextHeight {
	if (in < TextHeight::eMin) { return TextHeight::eMin; }
	if (in > TextHeight::eMax) { return TextHeight::eMax; }
	return in;
}
} // namespace spaced::graphics
