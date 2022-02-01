#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <levk/core/std_types.hpp>

namespace le::graphics {
template <typename T = f32, s64 Min = 0, s64 Max = 1>
struct TRect {
	using type = T;
	using type_float = std::conditional<std::is_floating_point_v<T>, T, f32>;

	static constexpr T min = T{Min};
	static constexpr T max = T{Max};
	static constexpr T half = (min + max) / 2;

	glm::tvec2<T> lt = {min, min};
	glm::tvec2<T> rb = {max, max};

	constexpr TRect() = default;
	constexpr TRect(glm::tvec2<T> const& lt, glm::tvec2<T> const& rb);
	constexpr TRect(glm::tvec4<T> const& ltrb);

	static constexpr TRect sizeTL(glm::tvec2<T> const& size, glm::tvec2<T> const& lt = glm::tvec2<T>(min)) noexcept;
	static constexpr TRect sizeCentre(glm::tvec2<T> const& size, glm::tvec2<T> const& c = glm::tvec2<T>(half)) noexcept;

	constexpr glm::tvec2<T> centre() const noexcept;
	constexpr glm::tvec2<T> size() const noexcept;
	constexpr type_float aspect() const noexcept;

	constexpr TRect<T, Min, Max> adjust(TRect const& by) const noexcept;
};

template <typename T, s64 Min, s64 Max>
constexpr TRect<T, Min, Max>::TRect(glm::tvec2<T> const& lt, glm::tvec2<T> const& rb) : lt(lt), rb(rb) {}

template <typename T, s64 Min, s64 Max>
constexpr TRect<T, Min, Max>::TRect(glm::tvec4<T> const& ltrb) : lt(ltrb.x, ltrb.y), rb(ltrb.z, ltrb.w) {}

template <typename T, s64 Min, s64 Max>
constexpr TRect<T, Min, Max> TRect<T, Min, Max>::sizeTL(glm::tvec2<T> const& size, glm::tvec2<T> const& leftTop) noexcept {
	return TRect({leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y + size.y});
}

template <typename T, s64 Min, s64 Max>
constexpr TRect<T, Min, Max> TRect<T, Min, Max>::sizeCentre(glm::tvec2<T> const& size, glm::tvec2<T> const& centre) noexcept {
	auto const leftTop = centre - (glm::tvec2<T>(0.5f) * size);
	return TRect({leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y + size.y});
}

template <typename T, s64 Min, s64 Max>
constexpr glm::tvec2<T> TRect<T, Min, Max>::centre() const noexcept {
	return {(rb.x + lt.x) * 0.5f, (rb.y + lt.y) * 0.5f};
}

template <typename T, s64 Min, s64 Max>
constexpr glm::tvec2<T> TRect<T, Min, Max>::size() const noexcept {
	return glm::tvec2<T>(rb.x - lt.x, rb.y - lt.y);
}

template <typename T, s64 Min, s64 Max>
constexpr typename TRect<T, Min, Max>::type_float TRect<T, Min, Max>::aspect() const noexcept {
	glm::tvec2<T> const s = size();
	return static_cast<type_float>(s.x) / static_cast<type_float>(s.y);
}

template <typename T, s64 Min, s64 Max>
constexpr TRect<T, Min, Max> TRect<T, Min, Max>::adjust(const TRect<T, Min, Max>& by) const noexcept {
	glm::tvec2<T> const s = size() * by.size();
	return TRect({lt.x + by.lt.x, lt.y + by.lt.y, lt.x + by.lt.x + s.x, lt.y + by.lt.y + s.y});
}

using ScreenRect = TRect<>;

struct ScreenView {
	ScreenRect nRect;
	glm::vec2 offset = {};
};
} // namespace le::graphics
