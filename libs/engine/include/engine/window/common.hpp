#pragma once
#include <array>
#include "core/std_types.hpp"
#include "core/zero.hpp"

namespace le
{
using WindowID = TZero<s32, -1>;

// Most desired in front
template <typename T>
using PriorityList = std::vector<T>;

enum class ColourSpace : u8
{
	eRGBLinear,
	eSRGBNonLinear,
	eCOUNT_,
};

enum class PresentMode : u8
{
	eFIFO,
	eMailbox,
	eCOUNT_
};

constexpr std::array colorSpaceNames = {"Unknown", "RGB Linear", "SRGB Non-linear"};
} // namespace le
