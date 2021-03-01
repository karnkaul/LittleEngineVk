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
  public:
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
	constexpr explicit Colour(u32 mask) noexcept {
		a = mask & 0xff;
		mask >>= 8;
		b = mask & 0xff;
		mask >>= 8;
		g = mask & 0xff;
		mask >>= 8;
		r = mask & 0xff;
	}
	///
	/// \brief Construct from a vec4 (XYZW => RGBA)
	///
	explicit Colour(glm::vec4 const& colour) noexcept;
	///
	/// \brief Construct from a vec3 (XYZ => RGB, A = 0xff)
	///
	explicit Colour(glm::vec3 const& colour) noexcept;
	///
	/// \brief Construct from a string mask
	///
	/// default: RGB => 0x0,  A => 0xff
	///
	explicit Colour(std::string_view hex) noexcept;

	///
	/// \brief Additive blend
	///
	Colour& operator+=(Colour rhs) noexcept;
	///
	/// \brief Additive blend
	///
	Colour& operator-=(Colour rhs) noexcept;

	///
	/// \brief Convert to GLSL format
	/// \returns RGBA normalised to [0.0f, 1.0f]
	///
	glm::vec4 toVec4() const noexcept;
	///
	/// \brief Convert to GLSL format
	/// \returns RGB normalised to [0.0f, 1.0f]
	///
	glm::vec3 toVec3() const noexcept;
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
	u32 toU32() const noexcept;
	///
	/// \brief Serialise as hex string
	///
	std::string toStr(bool bLeadingHash) const;
};

///
/// \brief Additive blend
///
Colour operator+(Colour lhs, Colour rhs) noexcept;
///
/// \brief Additive blend
///
Colour operator-(Colour lhs, Colour rhs) noexcept;
///
/// \brief Multiplicative blend
/// \param n Normalised (`[0.0f, 1.0f]`) blend fraction
/// \param colour Colour to blend by `n`
Colour& operator*=(f32 n, Colour& colour) noexcept;
///
/// \brief Multiplicative blend
/// \param colour
/// \param n: Normalised (`[0.0f, 1.0f]`) blend fraction
///
Colour operator*(f32 n, Colour colour) noexcept;

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
} // namespace le
