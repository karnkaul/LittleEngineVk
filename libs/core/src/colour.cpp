#include <algorithm>
#include <sstream>
#include <fmt/format.h>
#include <core/colour.hpp>
#include <core/ensure.hpp>
#include <core/maths.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>

namespace le {
Colour::Colour(std::string_view hex) noexcept {
	ENSURE(!hex.empty(), "Empty hex string!");
	if (hex[0] == '#') { hex = hex.substr(1); }
	std::string hexStr(hex);
	while (hexStr.length() < 3) { hexStr += "0"; }
	while (hexStr.length() < 8) { hexStr += "f"; }
	u32 mask = (u32)stoul(hexStr, nullptr, 16);
	a = mask & 0xff;
	mask >>= 8;
	b = mask & 0xff;
	mask >>= 8;
	g = mask & 0xff;
	mask >>= 8;
	r = mask & 0xff;
}

Colour Colour::lerp(Colour min, Colour max, f32 alpha) noexcept {
	if (alpha > 1.0f / 0xff) {
		auto lerpChannel = [&](UByte& out_l, UByte const& r) {
			if (out_l != r) { out_l = maths::lerp(out_l, r, alpha); }
		};
		lerpChannel(min.r, max.r);
		lerpChannel(min.g, max.g);
		lerpChannel(min.b, max.b);
		lerpChannel(min.a, max.a);
	}
	return min;
}

glm::vec4 Colour::toSRGB() const noexcept { return glm::convertLinearToSRGB(toVec4()); }

glm::vec4 Colour::toRGB() const noexcept { return glm::convertSRGBToLinear(toVec4()); }

std::string Colour::toString(bool leadingHash) const {
	std::stringstream str;
	str << std::hex << toU32();
	auto temp = str.str();
	std::string ret;
	ret.reserve(8);
	if (leadingHash) { ret += '#'; }
	for (s16 lead = (s16)(8U - temp.size()); lead > 0; --lead) { ret += '0'; }
	ret += std::move(temp);
	return ret;
}
} // namespace le
