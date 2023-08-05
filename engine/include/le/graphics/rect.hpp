#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace le::graphics {
template <typename Type = float>
struct Rect2D {
	glm::tvec2<Type> lt{};
	glm::tvec2<Type> rb{};

	static constexpr auto from_lbrt(glm::tvec2<Type> lb, glm::tvec2<Type> rt) -> Rect2D { return {.lt = {lb.x, rt.y}, .rb = {rt.x, lb.y}}; }

	static constexpr auto from_extent(glm::tvec2<Type> extent, glm::tvec2<Type> centre = {}) -> Rect2D {
		if (extent.x == Type{} && extent.y == Type{}) { return {.lt = centre, .rb = centre}; }
		auto const he = extent / Type{2};
		return {.lt = {centre.x - he.x, centre.y + he.y}, .rb = {centre.x + he.x, centre.y - he.y}};
	}

	[[nodiscard]] constexpr auto top_left() const -> glm::tvec2<Type> { return lt; }
	[[nodiscard]] constexpr auto top_right() const -> glm::tvec2<Type> { return {rb.x, lt.y}; }
	[[nodiscard]] constexpr auto bottom_left() const -> glm::tvec2<Type> { return {lt.x, rb.y}; }
	[[nodiscard]] constexpr auto bottom_right() const -> glm::tvec2<Type> { return rb; }

	[[nodiscard]] constexpr auto centre() const -> glm::tvec2<Type> { return {(lt.x + rb.x) / Type{2}, (lt.y + rb.y) / Type{2}}; }
	[[nodiscard]] constexpr auto extent() const -> glm::tvec2<Type> { return {rb.x - lt.x, lt.y - rb.y}; }

	[[nodiscard]] constexpr auto contains(glm::vec2 const point) const -> bool {
		return lt.x <= point.x && point.x <= rb.x && rb.y <= point.y && point.y <= lt.y;
	}

	constexpr auto operator*=(float const scale) -> Rect2D& {
		lt *= scale;
		rb *= scale;
		return *this;
	}

	friend constexpr auto operator*(float const scale, Rect2D const& rect) {
		auto ret = rect;
		ret *= scale;
		return ret;
	}

	auto operator==(Rect2D const&) const -> bool = default;
};

using OffsetRect = Rect2D<std::int32_t>;

using UvRect = Rect2D<float>;

inline constexpr UvRect uv_rect_v{.lt = {0.0f, 0.0f}, .rb = {1.0f, 1.0f}};
} // namespace le::graphics
