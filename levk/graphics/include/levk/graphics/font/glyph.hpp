#pragma once
#include <core/codepoint.hpp>
#include <levk/graphics/utils/quad_uv.hpp>

namespace le::graphics {
struct Glyph {
	QuadTex quad;
	glm::ivec2 topLeft{};
	glm::ivec2 advance{};
	Codepoint codepoint;
	bool textured{};
};
} // namespace le::graphics
