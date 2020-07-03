#pragma once
#include <glm/glm.hpp>
#include <core/std_types.hpp>
#include <core/ubyte.hpp>

namespace le
{
///
/// \brief Compressed wrapper struct for Colour
///
struct Colour
{
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
	static Colour lerp(Colour min, Colour max, f32 alpha);

	///
	/// \brief Default constructor
	///
	constexpr Colour() = default;
	///
	/// \brief Construct from a 32-bit mask (RGBA)
	///
	/// eg: 0xffff00ff for opaque yellow
	///
	constexpr explicit Colour(u32 mask) noexcept
	{
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
	explicit Colour(std::string_view hex);

	///
	/// \brief Additive blend
	///
	Colour& operator+=(Colour rhs);
	///
	/// \brief Additive blend
	///
	Colour& operator-=(Colour rhs);

	///
	/// \brief Convert to GLSL format
	/// \returns Each component normalised to `[0.0f, 1.0f]`
	///
	glm::vec4 toVec4() const;
	///
	/// \brief Convert colour space to sRGB
	///
	glm::vec4 toSRGB() const;
	///
	/// \brief Convert colour space to RGB
	///
	glm::vec4 toRGB() const;
};

///
/// \brief Additive blend
///
Colour operator+(Colour lhs, Colour rhs);
///
/// \brief Additive blend
///
Colour operator-(Colour lhs, Colour rhs);
///
/// \brief Multiplicative blend
/// \param n Normalised (`[0.0f, 1.0f]`) blend fraction
/// \param colour Colour to blend by `n`
Colour& operator*=(f32 n, Colour& colour);
///
/// \brief Multiplicative blend
/// \param colour
/// \param n: Normalised (`[0.0f, 1.0f]`) blend fraction
///
Colour operator*(f32 n, Colour colour);

bool operator==(Colour lhs, Colour rhs);
bool operator!=(Colour lhs, Colour rhs);

namespace colours
{
constexpr Colour black(0x000000ff);
constexpr Colour white(0xffffffff);
constexpr Colour red(0xff0000ff);
constexpr Colour green(0x00ff00ff);
constexpr Colour blue(0x0000ffff);
constexpr Colour yellow(0xffff00ff);
constexpr Colour magenta(0xff00ffff);
constexpr Colour cyan(0x00ffffff);
constexpr Colour transparent(0x0);
} // namespace colours
} // namespace le
