#pragma once
#include <glm/glm.hpp>
#include "std_types.hpp"
#include "ubyte.hpp"

namespace le
{
// \brief Compressed wrapper struct for SFML Color
struct Colour
{
public:
	static Colour lerp(Colour min, Colour max, f32 alpha);

	UByte r;
	UByte g;
	UByte b;
	UByte a;

	constexpr Colour() = default;
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
	explicit Colour(glm::vec4 const& colour) noexcept;
	explicit Colour(glm::vec3 const& colour) noexcept;
	explicit Colour(std::string_view hex);

	Colour& operator+=(Colour rhs);
	Colour& operator-=(Colour rhs);

	std::string toString() const;
	glm::vec4 toVec4() const;
	glm::vec4 toSRGB() const;
	glm::vec4 toRGB() const;
};

Colour operator+(Colour lhs, Colour rhs);
Colour operator-(Colour lhs, Colour rhs);
Colour& operator*=(f32 n, Colour& colour);
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
