#pragma once
#include <glm/trigonometric.hpp>

namespace levk {
struct Radians;

/// \brief Strong type representing degrees.
struct Degrees {
	/// \brief Value in degrees.
	float value{};

	/// \brief Implicit constructor from float.
	/// \param value value in degrees.
	/*implicit*/ constexpr Degrees(float value = {}) : value(value) {}
	/// \brief Implicit constructor from Radians.
	/// \param radians value in Radians.
	/*implicit*/ constexpr Degrees(Radians radians);

	/// \brief Convert to Radians.
	/// \returns Value in Radians.
	[[nodiscard]] constexpr auto to_radians() const -> Radians;

	constexpr operator float() const { return value; }
};

/// \brief Strong type representing radians.
struct Radians {
	/// \brief Value in radians.
	float value{};

	/// \brief Implicit constructor from float.
	/// \param value value in radians.
	/*implicit*/ constexpr Radians(float value = {}) : value(value) {}
	/// \brief Implicit constructor from Degrees.
	/// \param degrees value in Degrees.
	/*implicit*/ constexpr Radians(Degrees const degrees) : Radians(degrees.to_radians()) {}

	/// \brief Convert to Degrees.
	/// \returns Value in Degrees.
	[[nodiscard]] constexpr auto to_degrees() const -> Degrees { return {glm::degrees(value)}; }

	constexpr operator float() const { return value; }
};

constexpr auto Degrees::to_radians() const -> Radians { return {glm::radians(value)}; }
constexpr Degrees::Degrees(Radians const radians) : Degrees(radians.to_degrees()) {}
} // namespace levk
