#pragma once
#include <glm/trigonometric.hpp>
#include <cstdint>

namespace spaced {
struct Radians;

struct Degrees {
	float value{};

	constexpr Degrees(float value = {}) : value(value) {}
	constexpr Degrees(Radians radians);

	[[nodiscard]] constexpr auto to_radians() const -> Radians;

	constexpr operator float() const { return value; }
};

struct Radians {
	float value{};

	constexpr Radians(float value = {}) : value(value) {}
	constexpr Radians(Degrees degrees) : Radians(degrees.to_radians()) {}

	[[nodiscard]] constexpr auto to_degrees() const -> Degrees { return {glm::degrees(value)}; }

	constexpr operator float() const { return value; }
};

constexpr auto Degrees::to_radians() const -> Radians { return {glm::radians(value)}; }
constexpr Degrees::Degrees(Radians radians) : Degrees(radians.to_degrees()) {}
} // namespace spaced
