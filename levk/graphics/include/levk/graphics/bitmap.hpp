#pragma once
#include <glm/vec2.hpp>
#include <levk/core/span.hpp>
#include <levk/core/std_types.hpp>

namespace le::graphics {
using Extent2D = glm::uvec2;

template <typename T>
struct TBitmap {
	using type = T;

	static constexpr std::size_t channels = 4;

	T bytes{};
	Extent2D extent{};
};

using BmpBytes = std::vector<u8>;
using CubeBytes = std::array<BmpBytes, 6>;

using Bitmap = TBitmap<BmpBytes>;
using Cubemap = TBitmap<CubeBytes>;

using BmpView = Span<u8 const>;

using ImageData = Span<std::byte const>;
} // namespace le::graphics
