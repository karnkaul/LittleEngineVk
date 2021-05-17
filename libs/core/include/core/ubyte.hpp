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
	u8 value;

	// Any literals passed in must be positive!
	constexpr UByte(u8 value = 0) noexcept : value(value) {}
	constexpr explicit UByte(f32 nValue) noexcept : value((u8)(nValue * 0xff)) {}

	///
	/// \brief Obtain normalised value
	/// \returns Value in [0.0f, 1.0f]
	///
	constexpr f32 toF32() const noexcept { return value / (f32)0xff; }

	constexpr auto operator<=>(UByte const&) const = default;

	constexpr UByte operator+=(UByte rhs) noexcept;
	constexpr UByte operator-=(UByte rhs) noexcept;
	constexpr UByte operator*=(UByte rhs) noexcept;
	constexpr UByte operator/=(UByte rhs) noexcept;
};

constexpr UByte operator+(UByte lhs, UByte rhs) noexcept { return lhs += rhs; }
constexpr UByte operator-(UByte lhs, UByte rhs) noexcept { return lhs -= rhs; }
constexpr UByte operator*(UByte lhs, UByte rhs) noexcept { return lhs *= rhs; }
constexpr UByte operator/(UByte lhs, UByte rhs) noexcept { return lhs /= rhs; }
constexpr UByte operator*(f32 lhs, UByte rhs) noexcept { return UByte(u8(f32(rhs.value) * lhs)); }
constexpr UByte operator*(f32 lhs, UByte rhs) noexcept;

// impl

constexpr UByte UByte::operator+=(UByte rhs) noexcept {
	value = u8(s16(value) + s16(rhs.value));
	return *this;
}

constexpr UByte UByte::operator-=(UByte rhs) noexcept {
	value = u8(s16(value) - s16(rhs.value));
	return *this;
}

constexpr UByte UByte::operator*=(UByte rhs) noexcept {
	value = u8(s16(value) * s16(rhs.value));
	return *this;
}

constexpr UByte UByte::operator/=(UByte rhs) noexcept {
	value = u8(s16(value) * s16(rhs.value));
	return *this;
}
} // namespace le
