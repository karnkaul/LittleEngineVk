#pragma once
#include <core/codepoint.hpp>
#include <graphics/utils/extent2d.hpp>

namespace le::graphics {
struct Glyph {
	struct Metrics {
		glm::ivec2 bearing{};
		glm::ivec2 advance{};
	};

	Metrics metrics;
	Codepoint codepoint;
};
} // namespace le::graphics
