#pragma once
#include <engine/aabb.hpp>
#include <engine/render/flex.hpp>

namespace le::gui {
struct Rect : AABB {
	TFlex<glm::vec2> anchor;

	constexpr void offset(glm::vec2 size, glm::vec2 coeff = {1.0f, 1.0f}) noexcept { anchor.offset = (this->size = size) * 0.5f * coeff; }
	constexpr void adjust(Rect const& parent) noexcept { origin = parent.origin + (anchor.norm * parent.size) + anchor.offset; }
};
} // namespace le::gui
