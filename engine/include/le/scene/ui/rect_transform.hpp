#pragma once
#include <glm/mat4x4.hpp>

namespace le::ui {
struct RectTransform {
	glm::vec2 extent{200.0f};
	glm::vec2 position{};
	glm::vec2 anchor{};
	glm::vec2 scale{1.0f};

	[[nodiscard]] auto matrix() const -> glm::mat4;
};
} // namespace le::ui
