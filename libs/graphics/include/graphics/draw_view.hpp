#pragma once
#include <glm/vec2.hpp>
#include <graphics/screen_rect.hpp>

namespace le::graphics {
using DrawScissor = TRect<f32, 0, 1>;

struct DrawViewport : DrawScissor {
	glm::vec2 depth = {0.0f, 1.0f};
};
} // namespace le::graphics
