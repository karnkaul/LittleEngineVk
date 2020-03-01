#pragma once
#include <string>
#include "core/std_types.hpp"

namespace le
{
// \brief 1-byte unsigned integer structure, useful to store values between 0x00-ff
struct UByte
{
	u8 rawValue;

	// Any literals passed in must be positive!
	constexpr UByte(u8 value = 0) noexcept : rawValue(value) {}
	constexpr explicit UByte(f32 nValue) noexcept : rawValue((u8)(nValue * 0xff)) {}
	explicit UByte(std::string_view hex);

	u8 toU8() const;
	f32 toF32() const;

	UByte operator+=(UByte rhs);
	UByte operator-=(UByte rhs);
	UByte operator*=(UByte rhs);
	UByte operator/=(UByte rhs);
};

bool operator==(UByte lhs, UByte rhs);
bool operator!=(UByte lhs, UByte rhs);

UByte operator+(UByte lhs, UByte rhs);
UByte operator-(UByte lhs, UByte rhs);
UByte operator*(UByte lhs, UByte rhs);
UByte operator/(UByte lhs, UByte rhs);

UByte operator*(f32 lhs, UByte rhs);
} // namespace le
