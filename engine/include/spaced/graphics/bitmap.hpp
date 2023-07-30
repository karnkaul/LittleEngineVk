#pragma once
#include <glm/vec2.hpp>
#include <spaced/core/offset_span.hpp>
#include <cstdint>

namespace spaced::graphics {
template <typename StorageT, std::size_t Channels>
struct BasicBitmap {
	static constexpr auto channels_v = Channels;

	StorageT bytes{};
	glm::uvec2 extent{};
};

template <std::size_t Channels>
using BitmapByteSpan = BasicBitmap<OffsetSpan, Channels>;

template <std::size_t Channels>
using BitmapView = BasicBitmap<std::span<std::uint8_t const>, Channels>;

using Bitmap = BitmapView<4>;
} // namespace spaced::graphics
