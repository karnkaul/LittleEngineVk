#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include "core/fixed.hpp"

namespace le
{
u32 Fixed::toU32() const
{
	f64 val = toF64();
	if (val < 0)
	{
		val = -val;
	}
	u32 floor = u32(val);
	if ((floor * f64(SCALE_FACTOR)) - f64(value) >= 0.5)
	{
		return floor + 1;
	}
	return floor;
}

s32 Fixed::toS32() const
{
	s32 floor = s32(toF64());
	if ((floor * f64(SCALE_FACTOR)) - f64(value) >= 0.5)
	{
		return floor + 1;
	}
	return floor;
}

f32 Fixed::toF32() const
{
	return f32(value) / f32(SCALE_FACTOR);
}

f64 Fixed::toF64() const
{
	return f64(value) / f64(SCALE_FACTOR);
}

Fixed Fixed::power(f32 exponent) const
{
	Fixed ret = *this;
	if (exponent < 0)
	{
		ret = inverse();
		exponent = -exponent;
	}
	return Fixed(std::pow(ret.toF64(), exponent));
}

Fixed Fixed::sqrt() const
{
	return Fixed(std::sqrt(toF64()));
}

Fixed Fixed::inverse() const
{
	return Fixed(SCALE_FACTOR, value);
}

Fixed Fixed::sin() const
{
	return Fixed(std::sin(toF64()));
}

Fixed Fixed::arcSin() const
{
	f64 val = toF64();
	return Fixed(std::asin(val));
}

Fixed Fixed::cos() const
{
	return Fixed(std::cos(toF64()));
}

Fixed Fixed::arcCos() const
{
	f64 val = toF64();
	return Fixed(std::acos(val));
}

Fixed Fixed::tan() const
{
	return Fixed(std::tan(toF64()));
}

Fixed Fixed::arcTan() const
{
	return Fixed(std::atan(toF64()));
}
} // namespace le
