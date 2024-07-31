#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <cmath>

namespace levk {
/// \brief Axis aligned rectangle specified by top-left and bottom-right points.
template <typename Type = float>
struct Rect {
	/// \brief Left-top (x: left, y: top).
	glm::tvec2<Type> lt{};
	/// \brief Right-bottom (x: right, y: bottom).
	glm::tvec2<Type> rb{};

	/// \brief Construct from left-bottom and right-top.
	/// \param lb Left-bottom point.
	/// \param rt Right-top point.
	/// \returns Rect.
	static constexpr auto from_lbrt(glm::tvec2<Type> lb, glm::tvec2<Type> rt) -> Rect { return {.lt = {lb.x, rt.y}, .rb = {rt.x, lb.y}}; }

	/// \brief Construct from size and centre.
	/// \param size Total size of rect.
	/// \param centre Centre of rect.
	/// \returns Rect.
	static constexpr auto from_size(glm::tvec2<Type> size, glm::tvec2<Type> centre = {}) -> Rect {
		if (size.x == Type{} && size.y == Type{}) { return {.lt = centre, .rb = centre}; }
		auto const he = size / Type{2};
		return {.lt = {centre.x - he.x, centre.y + he.y}, .rb = {centre.x + he.x, centre.y - he.y}};
	}

	[[nodiscard]] constexpr auto top_left() const -> glm::tvec2<Type> { return lt; }
	[[nodiscard]] constexpr auto top_right() const -> glm::tvec2<Type> { return {rb.x, lt.y}; }
	[[nodiscard]] constexpr auto bottom_left() const -> glm::tvec2<Type> { return {lt.x, rb.y}; }
	[[nodiscard]] constexpr auto bottom_right() const -> glm::tvec2<Type> { return rb; }

	[[nodiscard]] constexpr auto centre() const -> glm::tvec2<Type> { return {(lt.x + rb.x) / Type{2}, (lt.y + rb.y) / Type{2}}; }
	[[nodiscard]] constexpr auto size() const -> glm::tvec2<Type> { return {std::abs(rb.x - lt.x), std::abs(lt.y - rb.y)}; }

	/// \brief Check if a point is contained within this rect.
	/// \param point Point to test against.
	/// \returns true if contained.
	[[nodiscard]] constexpr auto contains(glm::tvec2<Type> const point) const -> bool {
		return lt.x <= point.x && point.x <= rb.x && rb.y <= point.y && point.y <= lt.y;
	}

	/// \brief Check if any corner of another rect is contained within this rect.
	/// \param other Rect to test against.
	/// \returns true if any corner is contained.
	[[nodiscard]] constexpr auto contains(Rect<Type> const& other) const -> bool {
		return contains(other.top_left()) || contains(other.top_right()) || contains(other.bottom_left()) || contains(other.bottom_right());
	}

	template <typename T>
	constexpr operator Rect<T>() const {
		return {lt, rb};
	}

	constexpr auto operator*=(float const scale) -> Rect& {
		lt *= scale;
		rb *= scale;
		return *this;
	}

	friend constexpr auto operator*(float const scale, Rect const& rect) {
		auto ret = rect;
		ret *= scale;
		return ret;
	}

	auto operator==(Rect const&) const -> bool = default;
};

/// \brief Check if two rects are intersecting.
/// \param a First rect.
/// \param b Second rect.
/// \returns true if either rect contains the other.
template <typename Type>
[[nodiscard]] constexpr auto is_intersecting(Rect<Type> const& a, Rect<Type> const& b) -> bool {
	return a.contains(b) || b.contains(a);
}

/// \brief Alias for a rect in UV coordinates.
using UvRect = Rect<float>;

/// \brief Default UvRect (entire texture).
inline constexpr UvRect uv_rect_v{.lt = {0.0f, 0.0f}, .rb = {1.0f, 1.0f}};

/// \brief Convert a normalized point to UV coordinates.
/// \param n_xy Normalized point in world space (origin at centre, +y goes up).
/// \returns Point in UV space (origin at top-left, +y goes down).
[[nodiscard]] constexpr auto to_uv_coords(glm::vec2 const n_xy) -> glm::vec2 { return glm::vec2{0.5f + n_xy.x, 0.5f - n_xy.y}; }

/// \brief Convert a normalized Rect to UV coordinates.
/// \param n_rect Normalized rect in world space (origin at centre, +y goes up).
/// \returns Rect in UV space (origin at top-left, +y goes down).
[[nodiscard]] constexpr auto to_uv_rect(Rect<> const& n_rect) -> UvRect {
	return Rect<>{.lt = to_uv_coords(n_rect.top_left()), .rb = to_uv_coords(n_rect.bottom_right())};
}
} // namespace levk
