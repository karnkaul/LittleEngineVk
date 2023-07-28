#pragma once
#include <glm/gtx/quaternion.hpp>
#include <spaced/core/nvec3.hpp>
#include <spaced/core/radians.hpp>

namespace spaced {
///
/// \brief Identity quaternion.
///
inline constexpr auto quat_identity_v = glm::identity<glm::quat>();
///
/// \brief Identity matrix.
///
inline constexpr auto matrix_identity_v = glm::identity<glm::mat4>();

///
/// \brief Models an affine transformation in 3D space.
///
/// Provides friendly APIs for positioning, rotation, and scaling.
/// Caches combined 4x4 transformation matrix to avoid recomputing if unchanged.
///
class Transform {
  public:
	///
	/// \brief The front-end data representing a transformation.
	///
	struct Data {
		glm::vec3 position{};
		glm::quat orientation{quat_identity_v};
		glm::vec3 scale{1.0f};
	};

	///
	/// \brief Obtain a quaternion looking at point from eye.
	/// \param point Target point to look at
	/// \param eye Source position to look from
	/// \param start Starting orientation
	/// \param up Up vector
	///
	static auto look_at(glm::vec3 const& point, glm::vec3 const& eye, glm::quat const& start = glm::identity<glm::quat>(), nvec3 const& up = up_v) -> glm::quat;

	///
	/// \brief Obtain the transform data.
	/// \returns The transform data
	///
	auto data() const -> Data const& { return m_data; }
	///
	/// \brief Obtain the position.
	/// \returns The position
	///
	auto position() const -> glm::vec3 { return m_data.position; }
	///
	/// \brief Obtain the orientation.
	/// \returns The orientation
	///
	auto orientation() const -> glm::quat { return m_data.orientation; }
	///
	/// \brief Obtain the scale.
	/// \returns The scale
	///
	auto scale() const -> glm::vec3 { return m_data.scale; }

	///
	/// \brief Set the transform data.
	/// \param data The data to set to
	/// \returns Reference to self
	///
	auto set_data(Data data) -> Transform&;
	///
	/// \brief Set the position.
	/// \param position The position to set to
	/// \returns Reference to self
	///
	auto set_position(glm::vec3 position) -> Transform&;
	///
	/// \brief Set the orientation.
	/// \param orientation The orientation to set to
	/// \returns Reference to self
	///
	auto set_orientation(glm::quat orientation) -> Transform&;
	///
	/// \brief Set the scale.
	/// \param scale The scale to set to
	/// \returns Reference to self
	///
	auto set_scale(glm::vec3 scale) -> Transform&;

	///
	/// \brief Rotate the transform using an angle-axis approach.
	///
	///
	auto rotate(Radians radians, glm::vec3 const& axis) -> Transform&;
	///
	/// \brief Decompose the given matrix into the corresponding position, orientation, and scale.
	/// \param mat Matrix to decompose
	/// \returns Self
	///
	auto decompose(glm::mat4 const& mat) -> Transform&;

	static auto from(glm::mat4 const& mat) -> Transform {
		auto ret = Transform{};
		ret.decompose(mat);
		return ret;
	}

	///
	/// \brief Obtain the combined 4x4 transformation matrix.
	/// \returns The transformation matrix
	///
	auto matrix() const -> glm::mat4 const&;
	///
	/// \brief Recompute the transformation matrix.
	///
	void recompute() const;
	///
	/// \brief Check if the transformation matrix is stale / out of date.
	/// \returns true If the matrix needs to be recomputed
	///
	auto is_dirty() const -> bool { return m_dirty; }

	///
	/// \brief Obtain a transform that combines this with the passed parent.
	/// \param parent transformation matrix of the parent
	/// \returns Composition of parent and self
	///
	auto combined(glm::mat4 const& parent) const -> Transform;

  private:
	auto set_dirty() -> Transform&;

	mutable glm::mat4 m_matrix{matrix_identity_v};
	Data m_data{};
	mutable bool m_dirty{};
};

// impl

inline auto Transform::set_data(Data data) -> Transform& {
	m_data = data;
	return set_dirty();
}

inline auto Transform::set_position(glm::vec3 position) -> Transform& {
	m_data.position = position;
	return set_dirty();
}

inline auto Transform::set_orientation(glm::quat orientation) -> Transform& {
	m_data.orientation = glm::normalize(orientation);
	return set_dirty();
}

inline auto Transform::set_scale(glm::vec3 scale) -> Transform& {
	m_data.scale = scale;
	return set_dirty();
}

inline auto Transform::rotate(Radians radians, glm::vec3 const& axis) -> Transform& {
	m_data.orientation = glm::rotate(m_data.orientation, radians.value, axis);
	return set_dirty();
}

inline auto Transform::matrix() const -> glm::mat4 const& {
	if (is_dirty()) { recompute(); }
	return m_matrix;
}

inline void Transform::recompute() const {
	m_matrix = glm::translate(matrix_identity_v, m_data.position) * glm::toMat4(m_data.orientation) * glm::scale(matrix_identity_v, m_data.scale);
	m_dirty = false;
}

inline auto Transform::set_dirty() -> Transform& {
	m_dirty = true;
	return *this;
}
} // namespace spaced
