#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <core/tree.hpp>

namespace le
{
///
/// \brief Class for user-friendly and cached 4x4 matrix transformation with support for parenting
///
class Transform final : public Tree<Transform>
{
public:
	///
	/// \brief Identity transform (default constructed)
	///
	static Transform const s_identity;

public:
	///
	/// \brief Set (local) position
	///
	Transform& position(glm::vec3 const& position) noexcept;
	///
	/// \brief Set (local) orientation
	///
	Transform& orient(glm::quat const& orientation) noexcept;
	///
	/// \brief Set (local) scale
	///
	Transform& scale(f32 scale) noexcept;
	///
	/// \brief Set (local) scale
	///
	Transform& scale(glm::vec3 const& scale) noexcept;
	///
	/// \brief Reset to default state
	///
	void reset(bool bUnparent);

	///
	/// \brief Obtain local position
	///
	glm::vec3 const& position() const noexcept;
	///
	/// \brief Obtain local orientation
	///
	glm::quat const& orientation() const noexcept;
	///
	/// \brief Obtain local scale
	///
	glm::vec3 const& scale() const noexcept;
	///
	/// \brief Check if scale is uniform across all axes
	///
	bool isotropic() const noexcept;

	///
	/// \brief Obtain final position (after transformations from parent hierarchy)
	///
	glm::vec3 worldPosition() const noexcept;
	///
	/// \brief Obtain final orientation (after transformations from parent hierarchy)
	///
	glm::quat worldOrientation() const noexcept;
	///
	/// \brief Obtain final scale (after transformations from parent hierarchy)
	///
	glm::vec3 worldScale() const noexcept;

	///
	/// \brief Obtain transformation matrix (recompute if stale)
	///
	glm::mat4 model() const noexcept;
	///
	/// \brief Obtain normal matrix (recompute if stale)
	///
	glm::mat4 normalModel() const noexcept;

	///
	/// \brief Check if world transform is up to date
	///
	/// Returns `true` if any `Transform` in the hierarcy is stale
	///
	bool stale() const noexcept;
	///
	/// \brief Recompute transformation matrices if stale
	///
	void updateMats() const noexcept;

private:
	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	mutable glm::mat4 m_normalMat = glm::mat4(1.0f);
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 m_scale = glm::vec3(1.0f);
};

inline Transform const Transform::s_identity;

inline Transform& Transform::position(glm::vec3 const& position) noexcept
{
	m_position = position;
	m_bDirty = true;
	return *this;
}

inline Transform& Transform::orient(glm::quat const& orientation) noexcept
{
	m_orientation = orientation;
	m_bDirty = true;
	return *this;
}

inline Transform& Transform::scale(f32 scale) noexcept
{
	m_scale = {scale, scale, scale};
	m_bDirty = true;
	return *this;
}

inline Transform& Transform::scale(glm::vec3 const& scale) noexcept
{
	m_scale = scale;
	m_bDirty = true;
	return *this;
}

inline void Transform::reset(bool bUnparent)
{
	if (bUnparent)
	{
		purge();
	}
	m_position = {};
	m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	m_scale = glm::vec3(1.0f);
	updateMats();
}

inline glm::vec3 const& Transform::position() const noexcept
{
	return m_position;
}

inline glm::quat const& Transform::orientation() const noexcept
{
	return m_orientation;
}

inline glm::vec3 const& Transform::scale() const noexcept
{
	return m_scale;
}

inline bool Transform::isotropic() const noexcept
{
	return m_scale.x == m_scale.y && m_scale.y == m_scale.z && (!m_pParent || m_pParent->isotropic());
}

inline glm::vec3 Transform::worldPosition() const noexcept
{
	return glm::vec3(model()[3]);
}

inline glm::mat4 Transform::model() const noexcept
{
	updateMats();
	return m_pParent ? m_pParent->model() * m_mat : m_mat;
}

inline glm::mat4 Transform::normalModel() const noexcept
{
	updateMats();
	return m_normalMat;
}

inline bool Transform::stale() const noexcept
{
	return m_bDirty || (m_pParent ? m_pParent->stale() : false);
}

inline void Transform::updateMats() const noexcept
{
	if (m_bDirty)
	{
		static constexpr auto base = glm::mat4(1.0f);
		auto const t = glm::translate(base, m_position);
		auto const r = glm::toMat4(m_orientation);
		auto const s = glm::scale(base, m_scale);
		m_mat = t * r * s;
		m_normalMat = isotropic() ? m_mat : glm::mat4(glm::inverse(glm::transpose(glm::mat3(m_mat))));
		m_bDirty = false;
	}
	return;
}
} // namespace le
