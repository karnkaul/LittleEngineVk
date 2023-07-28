#pragma once
#include <glm/vec2.hpp>
#include <cstdint>
#include <span>

namespace spaced::graphics {
struct Bitmap {
	std::span<std::uint8_t const> bytes{};
	glm::uvec2 extent{};
	std::size_t channels{4};
};
} // namespace spaced::graphics
