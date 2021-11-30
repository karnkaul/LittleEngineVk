#pragma once
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <glm/vec2.hpp>

namespace le::graphics {
using Extent2D = glm::uvec2;

template <typename T>
struct TBitmap {
	using type = T;

	static constexpr std::size_t channels = 4;

	T bytes{};
	Extent2D size{};
};

using BmpBytes = std::vector<u8>;
using CubeBytes = std::array<BmpBytes, 6>;

using Bitmap = TBitmap<BmpBytes>;
using Cubemap = TBitmap<CubeBytes>;

using BmpView = Span<u8 const>;

using ImageData = Span<std::byte const>;
} // namespace le::graphics
