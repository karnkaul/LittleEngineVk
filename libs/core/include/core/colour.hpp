#pragma once
#include <core/std_types.hpp>
#include <core/ubyte.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace le {
///
/// \brief Compressed wrapper struct for Colour
///
struct Colour {
	UByte r;
	UByte g;
	UByte b;
	UByte a;

	///
	/// \brief Interpolate between two Colour instances
	/// \param alpha: normalised (`[0.0f, 1.0f]`) fraction of `max`
	/// \param max: Colour contributing to `alpha`
	/// \param min: Colour contributing to `1.0f - alpha`
	/// \returns interpolated Colour
	static Colour lerp(Colour min, Colour max, f32 alpha) noexcept;

	///
	/// \brief Default constructor
	///
	constexpr Colour() noexcept = default;
	///
	/// \brief Construct from a 32-bit mask (RGBA)
	///
	/// eg: 0xffff00ff for opaque yellow
	///
	constexpr Colour(u32 mask) noexcept : r(u8((mask >> 24) & 0xff)), g(u8((mask >> 16) & 0xff)), b(u8((mask >> 8) & 0xff)), a(u8(mask & 0xff)) {}
	///
	/// \brief Construct from a vec4 (XYZW => RGBA)
	///
	constexpr Colour(glm::vec3 const& colour) noexcept : Colour(glm::vec4(colour, 1.0f)) {}
	///
	/// \brief Construct from a vec3 (XYZ => RGB, A = 0xff)
	///
	constexpr Colour(glm::vec4 const& colour) noexcept : r(colour.x), g(colour.y), b(colour.z), a(colour.w) {}
	///
	/// \brief Construct from a string mask
	///
	/// default: RGB => 0x0,  A => 0xff
	///
	explicit Colour(std::string_view hex) noexcept;

	constexpr bool operator==(Colour const&) const = default;

	///
	/// \brief Additive blend
	///
	constexpr Colour& operator+=(Colour rhs) noexcept;
	///
	/// \brief Additive blend
	///
	constexpr Colour& operator-=(Colour rhs) noexcept;

	///
	/// \brief Convert to GLSL format
	/// \returns RGBA normalised to [0.0f, 1.0f]
	///
	constexpr glm::vec4 toVec4() const noexcept { return {r.toF32(), g.toF32(), b.toF32(), a.toF32()}; }
	///
	/// \brief Convert to GLSL format
	/// \returns RGB normalised to [0.0f, 1.0f]
	///
	constexpr glm::vec3 toVec3() const noexcept { return {r.toF32(), g.toF32(), b.toF32()}; }
	///
	/// \brief Convert colour space to sRGB
	///
	glm::vec4 toSRGB() const noexcept;
	///
	/// \brief Convert colour space to RGB
	///
	glm::vec4 toRGB() const noexcept;
	///
	/// \brief Convert to flat 32 bits
	///
	constexpr u32 toU32() const noexcept;
	///
	/// \brief Serialise as hex string
	///
	std::string toString(bool leadingHash) const;
};

///
/// \brief Additive blend
///
constexpr Colour operator+(Colour lhs, Colour rhs) noexcept { return lhs += rhs; }
///
/// \brief Additive blend
///
constexpr Colour operator-(Colour lhs, Colour rhs) noexcept { return lhs -= rhs; }
///
/// \brief Multiplicative blend
/// \param n Normalised (`[0.0f, 1.0f]`) blend fraction
/// \param colour Colour to blend by `n`
///
constexpr Colour& operator*=(f32 n, Colour& colour) noexcept;
///
/// \brief Multiplicative blend
/// \param colour
/// \param n: Normalised (`[0.0f, 1.0f]`) blend fraction
///
constexpr Colour operator*(f32 n, Colour colour) noexcept { return n *= colour; }

bool operator==(Colour lhs, Colour rhs) noexcept;
bool operator!=(Colour lhs, Colour rhs) noexcept;

namespace colours {
inline constexpr Colour black(0x000000ff);
inline constexpr Colour white(0xffffffff);
inline constexpr Colour red(0xff0000ff);
inline constexpr Colour green(0x00ff00ff);
inline constexpr Colour blue(0x0000ffff);
inline constexpr Colour yellow(0xffff00ff);
inline constexpr Colour magenta(0xff00ffff);
inline constexpr Colour cyan(0x00ffffff);
inline constexpr Colour transparent(0x0);
} // namespace colours

// impl

constexpr Colour& Colour::operator+=(Colour rhs) noexcept {
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;
	a += rhs.a;
	return *this;
}
constexpr Colour& Colour::operator-=(Colour rhs) noexcept {
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;
	a -= rhs.a;
	return *this;
}
constexpr u32 Colour::toU32() const noexcept {
	u32 raw = r.value;
	raw <<= 8;
	raw |= g.value;
	raw <<= 8;
	raw |= b.value;
	raw <<= 8;
	raw |= a.value;
	return raw;
}
constexpr Colour& operator*=(f32 n, Colour& c) noexcept {
	if (n < 0.0f) { n = 0.0f; }
	if (n > 1.0f) { n = 1.0f; }
	c.r = n * c.r;
	c.g = n * c.g;
	c.b = n * c.b;
	c.a = n * c.a;
	return c;
}
} // namespace le
