#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <cstdint>
#include <limits>
#include <string>

namespace levk {
template <typename Type, glm::length_t Channels>
using ColourT = glm::vec<Channels, Type>;

template <glm::length_t Channels>
using ColourF32 = ColourT<float, Channels>;
template <glm::length_t Channels>
using ColourU8 = ColourT<std::uint8_t, Channels>;

using RgbF32 = ColourF32<3>;
using RgbaF32 = ColourF32<4>;
using RgbU8 = ColourU8<3>;
using RgbaU8 = ColourU8<4>;

namespace colour {
inline constexpr auto max_channel_u8{std::numeric_limits<std::uint8_t>::max()};

constexpr auto blank_v = RgbaU8{0};
constexpr auto white_v = RgbaU8{max_channel_u8};
constexpr auto black_v = RgbaU8{0, 0, 0, max_channel_u8};
constexpr auto red_v = RgbaU8{max_channel_u8, 0, 0, max_channel_u8};
constexpr auto green_v = RgbaU8{0, max_channel_u8, 0, max_channel_u8};
constexpr auto blue_v = RgbaU8{0, 0, max_channel_u8, max_channel_u8};
constexpr auto yellow_v = RgbaU8{max_channel_u8, max_channel_u8, 0, max_channel_u8};
constexpr auto magenta_v = RgbaU8{max_channel_u8, 0, max_channel_u8, max_channel_u8};
constexpr auto cyan_v = RgbaU8{0, max_channel_u8, max_channel_u8, max_channel_u8};

/// \brief Convert counts of bits to bytes.
constexpr auto operator""_B(unsigned long long bit_count) { return bit_count * 8; }

/// \brief Convert an 8-bit channel to a normalized float.
/// \param channel 8-bit channel.
/// \returns Normalized float.
[[nodiscard]] constexpr auto to_f32(std::uint8_t const channel) -> float { return static_cast<float>(channel) / static_cast<float>(max_channel_u8); }
/// \brief Convert a normalized float into an 8-bit channel.
/// \param normalized Normalized float.
/// \returns 8-bit channel.
[[nodiscard]] constexpr auto to_u8(float const channel) -> std::uint8_t { return static_cast<std::uint8_t>(channel * static_cast<float>(max_channel_u8)); }

/// \brief Convert normalized floats into 8-bit channels.
/// \param normalized Channels of normalized [0-1] floats.
/// \returns 8-bit channels.
template <glm::length_t Channels>
[[nodiscard]] constexpr auto to_f32(ColourU8<Channels> const& in) -> ColourF32<Channels> {
	auto ret = ColourF32<Channels>{};
	ret.x = to_f32(in.x);
	if constexpr (Channels > 1) { ret.y = to_f32(in.y); }
	if constexpr (Channels > 2) { ret.z = to_f32(in.z); }
	if constexpr (Channels > 3) { ret.w = to_f32(in.w); }
	return ret;
}

/// \brief Convert 8-bit channels to normalized floats.
/// \param channel 8-bit channels.
/// \returns Normalized floats.
template <glm::length_t Channels>
[[nodiscard]] constexpr auto to_u8(ColourF32<Channels> const& in) -> ColourU8<Channels> {
	auto ret = ColourU8<Channels>{};
	ret.x = to_u8(in.x);
	if constexpr (Channels > 1) { ret.y = to_u8(in.y); }
	if constexpr (Channels > 2) { ret.z = to_u8(in.z); }
	if constexpr (Channels > 3) { ret.w = to_u8(in.w); }
	return ret;
}

/// \brief Construct an Rgb instance using hex encoding.
/// \param hex 32 bits encoded as RGBA.
/// \returns Rgb instance.
[[nodiscard]] constexpr auto from(std::uint32_t const hex) -> RgbaU8 {
	return {(hex >> 3_B) & max_channel_u8, (hex >> 2_B) & max_channel_u8, (hex >> 1_B) & max_channel_u8, hex & max_channel_u8};
}

/// \brief Convert a hex string to 8-bit RGBA.
[[nodiscard]] auto to_u8(std::string_view hex) -> RgbaU8;
/// \brief Convert 8-bit RGBA to its hex string.
[[nodiscard]] auto to_hex_string(RgbaU8 rgba) -> std::string;
/// \brief Convert 8-bit RGBA to its hex string.
[[nodiscard]] inline auto to_hex_string(RgbaF32 rgba) -> std::string { return to_hex_string(to_u8(rgba)); }

/// \brief Convert linear input to sRGB.
/// \param linear 4 normalized floats encoded as linear.
/// \returns 4 normalized floats encoded as sRGB.
[[nodiscard]] auto to_srgb(RgbaF32 const& linear) -> RgbaF32;
/// \brief Convert linear input to sRGB.
/// \param linear 8-bit channels encoded as linear.
/// \returns 4 normalized floats encoded as sRGB.
[[nodiscard]] inline auto to_srgb(RgbaU8 const& linear) -> RgbaF32 { return to_srgb(to_f32(linear)); }
/// \brief Convert sRGB input to linear.
/// \param srgb 4 normalized floats encoded as sRGB.
/// \returns 4 normalized floats encoded as linear.
[[nodiscard]] auto to_linear(RgbaF32 const& srgb) -> RgbaF32;
/// \brief Convert sRGB input to linear.
/// \param srgb 8-bit channels encoded as sRGB.
/// \returns 4 normalized floats encoded as linear.
[[nodiscard]] inline auto to_linear(RgbaU8 const& srgb) -> RgbaF32 { return to_linear(to_f32(srgb)); }
} // namespace colour
} // namespace levk
