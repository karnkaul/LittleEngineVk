#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>
#include <levk/colour.hpp>
#include <charconv>

namespace levk {
namespace {
[[nodiscard]] auto from_hex(std::string_view const hex) -> std::uint8_t {
	auto ret = std::uint8_t{};
	auto const* end = hex.data() + hex.size();
	auto const [ptr, ec] = std::from_chars(hex.data(), end, ret, 16);
	if (ptr != end || ec != std::errc{}) { return {}; }
	return ret;
}
} // namespace

auto colour::to_srgb(RgbaF32 const& linear) -> RgbaF32 { return glm::convertLinearToSRGB(linear); }
auto colour::to_linear(RgbaF32 const& srgb) -> RgbaF32 { return glm::convertSRGBToLinear(srgb); }

auto colour::to_u8(std::string_view hex) -> RgbaU8 {
	if (!hex.empty() && hex.front() == '#') { hex = hex.substr(1); }
	static constexpr std::string_view hex_v{"ffffffff"};
	if (hex.size() != hex_v.size()) { return {}; }

	auto out = RgbaU8{};
	out.x = from_hex(hex.substr(0, 2));
	out.y = from_hex(hex.substr(2, 2));
	out.z = from_hex(hex.substr(4, 2));
	out.w = from_hex(hex.substr(6));
	return out;
}

auto colour::to_hex_string(RgbaU8 const rgba) -> std::string { return std::format("#{:02x}{:02x}{:02x}{:02x}", rgba.x, rgba.y, rgba.z, rgba.w); }
} // namespace levk
