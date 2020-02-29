#pragma once
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "core/ubyte.hpp"

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
};

Colour operator+(Colour lhs, Colour rhs);
Colour operator-(Colour lhs, Colour rhs);
Colour& operator*=(f32 n, Colour& colour);
Colour operator*(f32 n, Colour colour);

bool operator==(Colour lhs, Colour rhs);
bool operator!=(Colour lhs, Colour rhs);

namespace colours
{
constexpr Colour Black(0x000000ff);
constexpr Colour White(0xffffffff);
constexpr Colour Red(0xff0000ff);
constexpr Colour Green(0x00ff00ff);
constexpr Colour Blue(0x0000ffff);
constexpr Colour Yellow(0xffff00ff);
constexpr Colour Magenta(0xff00ffff);
constexpr Colour Cyan(0x00ffffff);
constexpr Colour Transparent(0x0);
} // namespace colours
} // namespace le
