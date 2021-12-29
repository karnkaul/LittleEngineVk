#pragma once
#include <graphics/utils/extent2d.hpp>

namespace le::graphics {
struct QuadUV {
	glm::vec2 topLeft = {0.0f, 0.0f};
	glm::vec2 bottomRight = {1.0f, 1.0f};
};

struct QuadTex {
	QuadUV uv;
	Extent2D extent{};
};
} // namespace le::graphics
