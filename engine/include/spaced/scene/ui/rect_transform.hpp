#pragma once
#include <glm/mat4x4.hpp>
#include <spaced/graphics/rect.hpp>

namespace spaced::ui {
struct RectTransform {
	Rect2D<> frame{};
	glm::vec2 anchor{};
	glm::vec2 scale{1.0f};

	[[nodiscard]] constexpr auto merge_frame(Rect2D<> const& parent_frame) const -> Rect2D<> {
		auto const origin = parent_frame.centre();
		auto const extent = parent_frame.extent();
		auto const offset = origin + anchor * extent;
		auto ret = frame;
		ret.lt += offset;
		ret.rb += offset;
		return ret;
	}

	[[nodiscard]] auto matrix() const -> glm::mat4;
};
} // namespace spaced::ui
