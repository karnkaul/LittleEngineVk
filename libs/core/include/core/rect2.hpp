#pragma once
#include <glm/glm.hpp>
#include "core/std_types.hpp"

namespace le
{
struct Rect2
{
	glm::vec2 bl = glm::vec2(0.0f);
	glm::vec2 tr = glm::vec2(0.0f);

	constexpr Rect2() = default;
	Rect2(glm::vec2 const& size, glm::vec2 const& centre = glm::vec2(0.0f));

	glm::vec2 size() const;
	glm::vec2 centre() const;
	glm::vec2 tl() const;
	glm::vec2 br() const;
	f32 aspect() const;
};
} // namespace le
