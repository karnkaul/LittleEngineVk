#pragma once
#include <ostream>
#include "std_types.hpp"

namespace le
{
///
/// \brief Fixed point POD using constant integral scale factor
///
struct Fixed final
{
public:
	static constexpr u32 s_scaleFactor = 10000;

private:
	s32 value;

public:
	constexpr Fixed() noexcept : value(0) {}
	constexpr explicit Fixed(f32 value) noexcept : value(s32(value * s_scaleFactor)) {}
	constexpr explicit Fixed(f64 value) noexcept : value(s32(value * s_scaleFactor)) {}
	constexpr Fixed(s32 numerator, s32 denominator = 1) noexcept : value(s32(s64(numerator) * s64(s_scaleFactor) / s64(denominator))){};

	u32 toU32() const;
	s32 toS32() const;
	f32 toF32() const;
	f64 toF64() const;

	Fixed& operator+=(Fixed rhs);
	Fixed& operator-=(Fixed rhs);
	Fixed& operator*=(Fixed rhs);
	Fixed& operator/=(Fixed rhs);
	Fixed& operator%=(Fixed rhs);
	Fixed& operator++();
	Fixed operator++(int);
	Fixed& operator--();
	Fixed operator--(int);

	Fixed abs() const;
	Fixed power(f32 exponent) const;
	Fixed sqrt() const;
	Fixed inverse() const;
	Fixed sin() const;
	Fixed arcSin() const;
	Fixed cos() const;
	Fixed arcCos() const;
	Fixed tan() const;
	Fixed arcTan() const;

	bool operator==(Fixed rhs) const;
	bool operator>(Fixed rhs) const;
	bool operator<(Fixed rhs) const;
	bool operator<=(Fixed rhs) const;
	bool operator>=(Fixed rhs) const;

private:
	friend Fixed operator-(Fixed rhs);
};

std::ostream& operator<<(std::ostream& os, Fixed rhs);
Fixed operator+(Fixed lhs, Fixed rhs);
Fixed operator-(Fixed lhs, Fixed rhs);
Fixed operator*(Fixed lhs, Fixed rhs);
Fixed operator/(Fixed lhs, Fixed rhs);
Fixed operator%(Fixed lhs, Fixed rhs);
bool operator!=(Fixed lhs, Fixed rhs);

inline Fixed& Fixed::operator+=(Fixed rhs)
{
	value += rhs.value;
	return *this;
}

inline Fixed& Fixed::operator-=(Fixed rhs)
{
	value -= rhs.value;
	return *this;
}

inline Fixed& Fixed::operator*=(Fixed rhs)
{
	s64 largeVal = static_cast<s64>(value) * static_cast<s64>(rhs.value);
	value = s32(largeVal / s_scaleFactor);
	return *this;
}

inline Fixed& Fixed::operator/=(Fixed rhs)
{
	if (rhs.value == 0)
	{
		value = 0;
	}
	else
	{
		s64 largeVal = static_cast<s64>(value) * s_scaleFactor;
		value = s32(largeVal / rhs.value);
	}
	return *this;
}

inline Fixed& Fixed::operator%=(Fixed rhs)
{
	value %= rhs.value;
	return *this;
}

inline Fixed& Fixed::operator++()
{
	++value;
	return *this;
}

inline Fixed Fixed::operator++(int)
{
	Fixed temporary(*this);
	++*this;
	return temporary;
}

inline Fixed& Fixed::operator--()
{
	--value;
	return *this;
}

inline Fixed Fixed::operator--(int)
{
	Fixed temporary(*this);
	--*this;
	return temporary;
}

inline Fixed Fixed::abs() const
{
	return (value < 0) ? -*this : *this;
}

inline bool Fixed::operator==(Fixed rhs) const
{
	return value == rhs.value;
}

inline bool Fixed::operator>(Fixed rhs) const
{
	return value > rhs.value;
}

inline bool Fixed::operator<(Fixed rhs) const
{
	return value < rhs.value;
}

inline bool Fixed::operator<=(Fixed rhs) const
{
	return value <= rhs.value;
}

inline bool Fixed::operator>=(Fixed rhs) const
{
	return value >= rhs.value;
}

inline Fixed operator-(Fixed rhs)
{
	Fixed ret;
	ret.value = -rhs.value;
	return ret;
}

inline std::ostream& operator<<(std::ostream& os, Fixed rhs)
{
	return os << rhs.toF64();
}

inline Fixed operator+(Fixed lhs, Fixed rhs)
{
	return Fixed(lhs) += rhs;
}

inline Fixed operator-(Fixed lhs, Fixed rhs)
{
	return Fixed(lhs) -= rhs;
}

inline Fixed operator*(Fixed lhs, Fixed rhs)
{
	return Fixed(lhs) *= rhs;
}

inline Fixed operator/(Fixed lhs, Fixed rhs)
{
	return Fixed(lhs) /= rhs;
}

inline Fixed operator%(Fixed lhs, Fixed rhs)
{
	return Fixed(lhs) %= rhs;
}

inline bool operator!=(Fixed lhs, Fixed rhs)
{
	return !(lhs == rhs);
}
} // namespace le
