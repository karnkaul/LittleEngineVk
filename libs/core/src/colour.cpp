#include <sstream>
#include "core/assert.hpp"
#include "core/colour.hpp"
#include "core/fmt.hpp"
#include "core/maths.hpp"

namespace le
{
Colour::Colour(glm::vec3 const& colour) noexcept : Colour(glm::vec4(colour, 1.0f)) {}

Colour::Colour(glm::vec4 const& colour) noexcept : r(colour.r), g(colour.g), b(colour.b), a(colour.a) {}

Colour::Colour(std::string_view hex)
{
	ASSERT(!hex.empty(), "Empty hex string!");
	if (hex.at(0) == '#')
	{
		hex = hex.substr(1);
	}
	std::string hexStr(hex);
	while (hexStr.length() < 3)
	{
		hexStr += "0";
	}
	while (hexStr.length() < 8)
	{
		hexStr += "f";
	}
	u32 mask = (u32)stoul(hexStr, nullptr, 16);
	a = mask & 0xff;
	mask >>= 8;
	b = mask & 0xff;
	mask >>= 8;
	g = mask & 0xff;
	mask >>= 8;
	r = mask & 0xff;
}

Colour Colour::lerp(Colour min, Colour max, f32 alpha)
{
	if (alpha > 1.0f / 0xff)
	{
		auto lerpChannel = [&](UByte& out_l, UByte const& r) {
			if (out_l != r)
			{
				out_l = maths::lerp(out_l, r, alpha);
			}
		};
		lerpChannel(min.r, max.r);
		lerpChannel(min.g, max.g);
		lerpChannel(min.b, max.b);
		lerpChannel(min.a, max.a);
	}
	return min;
}

Colour& Colour::operator+=(Colour rhs)
{
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;
	a += rhs.a;
	return *this;
}

Colour& Colour::operator-=(Colour rhs)
{
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;
	a -= rhs.a;
	return *this;
}

std::string Colour::toString() const
{
	return fmt::format("{}, {}, {}, {}", r.rawValue, g.rawValue, b.rawValue, a.rawValue);
}

Colour operator+(Colour lhs, Colour rhs)
{
	return lhs += rhs;
}

Colour operator-(Colour lhs, Colour rhs)
{
	return lhs -= rhs;
}

Colour& operator*=(f32 n, Colour& c)
{
	n = maths::clamp(n, 0.0f, 1.0f);
	c.r = n * c.r;
	c.g = n * c.g;
	c.b = n * c.b;
	c.a = n * c.a;
	return c;
}

Colour operator*(f32 n, Colour colour)
{
	return n *= colour;
}

bool operator==(Colour lhs, Colour rhs)
{
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

bool operator!=(Colour lhs, Colour rhs)
{
	return !(lhs == rhs);
}
} // namespace le
