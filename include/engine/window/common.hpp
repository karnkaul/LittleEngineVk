#pragma once
#include <array>
#include <core/std_types.hpp>
#include <core/utils.hpp>
#include <core/zero.hpp>

namespace le
{
using WindowID = TZero<s32, -1>;

// Most desired in front
template <typename T>
using PriorityList = Span<T>;

enum class ColourSpace : s8
{
	eSRGBNonLinear,
	eRGBLinear,
	eCOUNT_,
};

enum class PresentMode : s8
{
	eImmediate,
	eMailbox,
	eFifo,
	eFifoRelaxed,
	eCOUNT_
};
} // namespace le
