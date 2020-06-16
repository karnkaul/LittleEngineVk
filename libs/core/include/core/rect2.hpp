#pragma once
#include <glm/glm.hpp>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Wrapper struct for storing 2D rect info
///
struct Rect2
{
	///
	/// \brief Bottom Left
	///
	glm::vec2 bl = glm::vec2(0.0f);
	///
	/// \brief Top Right
	///
	glm::vec2 tr = glm::vec2(0.0f);

	constexpr Rect2() = default;
	Rect2(glm::vec2 const& size, glm::vec2 const& centre = glm::vec2(0.0f));

	///
	/// \brief Obtain size of rect
	///
	glm::vec2 size() const;
	///
	/// \brief Obtain centre of rect
	///
	glm::vec2 centre() const;
	///
	/// \brief Obtain Top Left
	///
	glm::vec2 tl() const;
	///
	/// \brief Obtain Bottom Right
	///
	glm::vec2 br() const;
	///
	/// \brief Obtain aspect ratio
	///
	f32 aspect() const;
};
} // namespace le
