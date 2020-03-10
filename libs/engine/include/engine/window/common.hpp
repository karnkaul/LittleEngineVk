#pragma once
#include <array>
#include "core/std_types.hpp"
#include "core/zero.hpp"

namespace le
{
using WindowID = TZero<s32, -1>;

enum class ColourSpace : u8
{
	eDontCare = 0,
	eRGBLinear,
	eSRGBNonLinear,
	eCOUNT_,
};

constexpr std::array colorSpaceNames = {"Unknown", "RGB Linear", "SRGB Non-linear"};
} // namespace le
