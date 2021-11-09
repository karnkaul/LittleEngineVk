#pragma once
#include <engine/input/space.hpp>

namespace le::scene {
struct Space {
	enum class Type { eSwapchain, eRenderer, eCustom };

	glm::vec2 custom{};
	Type type = Type::eSwapchain;

	glm::vec2 operator()(input::Space const& space) const noexcept {
		if (type == Type::eCustom && custom.x > 0.0f && custom.y > 0.0f) { return custom; }
		if (type == Type::eRenderer) { return space.render.area; }
		return space.display.swapchain;
	}
};
} // namespace le::scene
