#pragma once
#include <array>
#include <core/std_types.hpp>
#include <core/zero.hpp>

namespace le
{
using WindowID = TZero<s32, -1>;

// Most desired in front
template <typename T>
using PriorityList = std::vector<T>;

enum class ColourSpace : s8
{
	eSRGBNonLinear,
	eRGBLinear,
	eCOUNT_,
};

enum class PresentMode : s8
{
	eFIFO,
	eMailbox,
	eImmediate,
	eCOUNT_
};

[[maybe_unused]] constexpr std::array colorSpaceNames = {"Unknown", "RGB Linear", "SRGB Non-linear"};
} // namespace le
