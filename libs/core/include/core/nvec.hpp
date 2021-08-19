#pragma once
#include <core/maths.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace le {
constexpr f32 lengthSq(glm::vec2 vec) noexcept { return vec.x * vec.x + vec.y + vec.y; }
constexpr f32 lengthSq(glm::vec3 const& vec) noexcept { return vec.x * vec.x + vec.y + vec.y + vec.z * vec.z; }

template <typename T>
constexpr bool isNormalized(T const& vec, f32 epsilon = 0.001f) noexcept {
	return maths::equals(lengthSq(vec), 1.0f, epsilon);
}

///
/// \brief Obtain normalized vector
///
template <typename T>
T normalized(T const& t) noexcept {
	return t / std::sqrt(lengthSq(t));
}

///
/// \brief glm::vec2 wrapper that normalizes on constructiom
///
struct nvec2 : glm::vec2 {
	nvec2(f32 x, f32 y) noexcept : glm::vec2(x, y) { static_cast<glm::vec2&>(*this) = normalized(static_cast<glm::vec2 const&>(*this)); }
	explicit nvec2(glm::vec2 vec) noexcept : nvec2(vec.x, vec.y) {}

	bool operator==(nvec2 const&) const = default;

	nvec2& operator*=(nvec2 rhs) noexcept {
		static_cast<glm::vec2&>(*this) *= static_cast<glm::vec2 const&>(rhs);
		return *this;
	}

	friend nvec2 operator*(nvec2 lhs, nvec2 rhs) noexcept { return lhs *= rhs; }
};

///
/// \brief glm::vec3 wrapper that normalizes on constructiom
///
struct nvec3 : glm::vec3 {
	nvec3(f32 x, f32 y, f32 z) noexcept : glm::vec3(x, y, z) { static_cast<glm::vec3&>(*this) = normalized(static_cast<glm::vec3 const&>(*this)); }
	explicit nvec3(glm::vec3 const& vec) noexcept : nvec3(vec.x, vec.y, vec.z) {}

	bool operator==(nvec3 const&) const = default;

	nvec3& operator*=(nvec3 const& rhs) noexcept {
		static_cast<glm::vec3&>(*this) *= static_cast<glm::vec3 const&>(rhs);
		return *this;
	}

	friend nvec3 operator*(nvec3 lhs, nvec3 rhs) noexcept { return lhs *= rhs; }
};
} // namespace le
