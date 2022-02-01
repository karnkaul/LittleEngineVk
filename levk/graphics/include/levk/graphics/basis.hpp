#pragma once
#include <glm/gtx/quaternion.hpp>
#include <levk/core/nvec.hpp>

namespace le::graphics {
constexpr glm::vec3 up = {0.0f, 1.0f, 0.0f};
constexpr glm::vec3 right = {1.0f, 0.0f, 0.0f};
constexpr glm::vec3 front = {0.0f, 0.0f, 1.0f};
constexpr glm::quat identity = {1.0f, 0.0f, 0.0f, 0.0f};

template <typename T>
constexpr glm::tvec2<T> invertY(glm::tvec2<T> vec) noexcept {
	return glm::tvec2<T>(vec.x, -vec.y);
}

template <typename T>
constexpr glm::tvec3<T> invertY(glm::tvec3<T> vec) noexcept {
	return glm::tvec3<T>(vec.x, -vec.y, vec.z);
}
} // namespace le::graphics
