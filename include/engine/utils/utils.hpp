#pragma once
#include <glm/vec2.hpp>

namespace le::utils {
constexpr bool inAABB(glm::vec2 point, glm::vec2 tl, glm::vec2 br) noexcept { return point.x >= tl.x && point.x <= br.x && point.y >= br.y && point.y <= tl.y; }
} // namespace le::utils
