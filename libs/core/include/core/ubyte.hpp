#pragma once
#include <string>
#include <core/std_types.hpp>

namespace le {
///
/// \brief Single byte unsigned integer structure
///
/// Useful for storing values between 0x00-0xff
///
struct UByte {
	u8 rawValue;

	// Any literals passed in must be positive!
	constexpr UByte(u8 value = 0) noexcept : rawValue(value) {}
	constexpr explicit UByte(f32 nValue) noexcept : rawValue((u8)(nValue * 0xff)) {}
	explicit UByte(std::string_view hex);

	u8 toU8() const noexcept;
	///
	/// \brief Obtain normalised value
	/// \returns Value in [0.0f, 1.0f]
	///
	f32 toF32() const noexcept;

	UByte operator+=(UByte rhs) noexcept;
	UByte operator-=(UByte rhs) noexcept;
	UByte operator*=(UByte rhs) noexcept;
	UByte operator/=(UByte rhs) noexcept;
};

bool operator==(UByte lhs, UByte rhs) noexcept;
bool operator!=(UByte lhs, UByte rhs) noexcept;

UByte operator+(UByte lhs, UByte rhs) noexcept;
UByte operator-(UByte lhs, UByte rhs) noexcept;
UByte operator*(UByte lhs, UByte rhs) noexcept;
UByte operator/(UByte lhs, UByte rhs) noexcept;

UByte operator*(f32 lhs, UByte rhs) noexcept;
} // namespace le
