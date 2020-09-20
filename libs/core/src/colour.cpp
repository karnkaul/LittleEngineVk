#include <algorithm>
#include <sstream>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <core/assert.hpp>
#include <core/colour.hpp>
#include <core/maths.hpp>

namespace le
{
Colour::Colour(glm::vec3 const& colour) noexcept : Colour(glm::vec4(colour, 1.0f)) {}

Colour::Colour(glm::vec4 const& colour) noexcept : r(colour.r), g(colour.g), b(colour.b), a(colour.a) {}

Colour::Colour(std::string_view hex) noexcept
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

Colour Colour::lerp(Colour min, Colour max, f32 alpha) noexcept
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

Colour& Colour::operator+=(Colour rhs) noexcept
{
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;
	a += rhs.a;
	return *this;
}

Colour& Colour::operator-=(Colour rhs) noexcept
{
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;
	a -= rhs.a;
	return *this;
}

glm::vec4 Colour::toVec4() const noexcept
{
	return {r.toF32(), g.toF32(), b.toF32(), a.toF32()};
}

glm::vec4 Colour::toSRGB() const noexcept
{
	return glm::convertLinearToSRGB(toVec4());
}

glm::vec4 Colour::toRGB() const noexcept
{
	return glm::convertSRGBToLinear(toVec4());
}

u32 Colour::toU32() const noexcept
{
	u32 raw = r.toU8();
	raw <<= 8;
	raw |= g.toU8();
	raw <<= 8;
	raw |= b.toU8();
	raw <<= 8;
	raw |= a.toU8();
	return raw;
}

std::string Colour::toStr(bool bLeadingHash) const
{
	std::stringstream str;
	str << std::hex << toU32();
	auto temp = str.str();
	std::string ret;
	ret.reserve(8);
	if (bLeadingHash)
	{
		ret += '#';
	}
	for (s16 lead = (s16)(8U - temp.size()); lead > 0; --lead)
	{
		ret += '0';
	}
	ret += std::move(temp);
	return ret;
}

Colour operator+(Colour lhs, Colour rhs) noexcept
{
	return lhs += rhs;
}

Colour operator-(Colour lhs, Colour rhs) noexcept
{
	return lhs -= rhs;
}

Colour& operator*=(f32 n, Colour& c) noexcept
{
	n = std::clamp(n, 0.0f, 1.0f);
	c.r = n * c.r;
	c.g = n * c.g;
	c.b = n * c.b;
	c.a = n * c.a;
	return c;
}

Colour operator*(f32 n, Colour colour) noexcept
{
	return n *= colour;
}

bool operator==(Colour lhs, Colour rhs) noexcept
{
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

bool operator!=(Colour lhs, Colour rhs) noexcept
{
	return !(lhs == rhs);
}
} // namespace le
