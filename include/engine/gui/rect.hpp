#pragma once
#include <core/std_types.hpp>
#include <engine/render/flex.hpp>
#include <engine/utils/utils.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace le::gui {
struct Rect {
	TFlex<glm::vec2> anchor;
	glm::vec2 origin = {};
	glm::vec2 size = {};

	constexpr void offset(glm::vec2 size, glm::vec2 coeff = {1.0f, 1.0f}) noexcept { anchor.offset = (this->size = size) * 0.5f * coeff; }
	constexpr glm::vec3 position(f32 zIndex = 0.0f) const noexcept { return {origin, zIndex}; }
	constexpr bool hit(glm::vec2 point) const noexcept { return hit(origin, halfSize(), point); }
	constexpr glm::vec2 halfSize() const noexcept { return {size.x * 0.5f, -size.y * 0.5f}; }
	constexpr void adjust(Rect const& parent) noexcept { origin = parent.origin + (anchor.norm * parent.size) + anchor.offset; }

	static constexpr bool hit(glm::vec2 centre, glm::vec2 half, glm::vec2 point) noexcept { return utils::inAABB(point, centre - half, centre + half); }
};
} // namespace le::gui
