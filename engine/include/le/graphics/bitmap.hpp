#pragma once
#include <glm/vec2.hpp>
#include <le/core/offset_span.hpp>
#include <cstdint>

namespace le::graphics {
template <typename StorageT, std::size_t Channels>
struct BasicBitmap {
	static constexpr auto channels_v = Channels;

	StorageT bytes{};
	glm::uvec2 extent{};
};

template <std::size_t Channels>
using BitmapByteSpan = BasicBitmap<OffsetSpan, Channels>;

template <std::size_t Channels>
using BitmapView = BasicBitmap<std::span<std::byte const>, Channels>;

using Bitmap = BitmapView<4>;
} // namespace le::graphics
