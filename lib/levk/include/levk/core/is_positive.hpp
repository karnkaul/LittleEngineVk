#pragma once
#include <glm/vec2.hpp>

namespace levk {
/// \brief Check whether a given extent (size) is positive.
/// \param extent Input size.
/// \returns true if both x and y positive.
template <typename Type>
[[nodiscard]] constexpr auto is_positive(glm::tvec2<Type> const extent) -> bool {
	return extent.x > Type{0} && extent.y > Type{0};
}
} // namespace levk
