#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace le::graphics {
///
/// \brief 4-channel colour with HDR support.
///
struct Rgba {
	static constexpr std::uint8_t max_v{0xff};

	///
	/// \brief 8-bit channels.
	///
	glm::tvec4<std::uint8_t> channels{max_v, max_v, max_v, max_v};
	///
	/// \brief Convert an 8-bit channels to a normalized float.
	/// \param channel 8-bit channel
	/// \returns Normalized float
	///
	static constexpr auto to_f32(std::uint8_t channel) -> float { return static_cast<float>(channel) / static_cast<float>(max_v); }
	///
	/// \brief Convert a normalized float into an 8-bit channel.
	/// \param normalized Normalized float
	/// \returns 8-bit channel
	///
	static constexpr auto to_u8(float normalized) -> std::uint8_t { return static_cast<std::uint8_t>(normalized * static_cast<float>(max_v)); }
	///
	/// \brief Convert linear input to sRGB.
	/// \param linear 4 normalized floats encoded as linear
	/// \returns 4 normalized floats encoded as sRGB
	///
	static auto to_srgb(glm::vec4 const& linear) -> glm::vec4;
	///
	/// \brief Convert sRGB input to linear.
	/// \param srgb 4 normalized floats encoded as sRGB
	/// \returns 4 normalized floats encoded as linear
	///
	static auto to_linear(glm::vec4 const& srgb) -> glm::vec4;

	///
	/// \brief Construct an Rgb instance using normalized input and intensity.
	/// \param normalized 3 channels of normalized [0-1] floats
	/// \returns Rgb instance
	///
	static constexpr auto from(glm::vec4 const& normalized) -> Rgba {
		return {.channels = {to_u8(normalized.x), to_u8(normalized.y), to_u8(normalized.z), to_u8(normalized.w)}};
	}

	///
	/// \brief Obtain only the normalzed tint (no HDR).
	///
	[[nodiscard]] constexpr auto to_tint() const -> glm::vec4 {
		return glm::vec4{to_f32(channels.x), to_f32(channels.y), to_f32(channels.z), to_f32(channels.w)};
	}

	///
	/// \brief Convert instance to 4 channel normalized output.
	/// \returns 4 normalized floats
	///
	[[nodiscard]] constexpr auto to_vec4(float intensity = 1.0f) const -> glm::vec4 { return glm::vec4{intensity * glm::vec3{to_tint()}, to_f32(channels.w)}; }

	auto operator==(Rgba const&) const -> bool = default;
};

struct HdrRgba : Rgba {
	///
	/// \brief Intensity (> 1.0 == HDR).
	///
	float intensity{1.0f};

	HdrRgba() = default;
	constexpr HdrRgba(float intensity) : intensity(intensity) {}
	constexpr HdrRgba(Rgba rgba, float intensity) : Rgba{rgba.channels}, intensity(intensity) {}

	///
	/// \brief Construct an Rgb instance using normalized input and intensity.
	/// \param normalized 3 channels of normalized [0-1] floats
	/// \param intensity Intensity of returned Rgb
	/// \returns Rgb instance
	///
	static constexpr auto from(glm::vec4 const& normalized, float intensity = 1.0f) -> HdrRgba { return {Rgba::from(normalized), intensity}; }

	///
	/// \brief Convert instance to 4 channel normalized output.
	/// \returns 4 normalized floats
	///
	[[nodiscard]] constexpr auto to_vec4() const -> glm::vec4 { return Rgba::to_vec4(intensity); }
};

constexpr auto blank_v = Rgba{{0x0, 0x0, 0x0, 0x0}};
constexpr auto white_v = Rgba{{0xff, 0xff, 0xff, 0xff}};
constexpr auto black_v = Rgba{{0x0, 0x0, 0x0, 0xff}};
constexpr auto red_v = Rgba{{0xff, 0x0, 0x0, 0xff}};
constexpr auto green_v = Rgba{{0x0, 0xff, 0x0, 0xff}};
constexpr auto blue_v = Rgba{{0x0, 0x0, 0xff, 0xff}};
constexpr auto yellow_v = Rgba{{0xff, 0xff, 0x0, 0xff}};
constexpr auto magenta_v = Rgba{{0xff, 0x0, 0xff, 0xff}};
constexpr auto cyan_v = Rgba{{0x0, 0xff, 0xff, 0xff}};
} // namespace le::graphics
