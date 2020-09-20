#pragma once
#include <list>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Class for user-friendly and cached 4x4 matrix transformation with support for parenting
///
class Transform final
{
public:
	///
	/// \brief Identity transform (default constructed)
	///
	static Transform const s_identity;

public:
	Transform();
	~Transform();

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
	/// \brief Set `pParent` as the parent of this transform
	///
	/// Pass `nullptr` to unset
	///
	Transform& parent(Transform* pParent);
	///
	/// \brief Reset to default state
	///
	void reset(bool bUnparent);

	///
	/// \brief Obtain parent (if any)
	///
	Transform const* parent() const noexcept;
	///
	/// \brief Obtain parent (if any)
	///
	Transform* parent() noexcept;
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
	///
	/// \brief Obtain children parented to this Transform
	///
	std::list<Ref<Transform>> const& children() const noexcept;

private:
	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	mutable glm::mat4 m_normalMat = glm::mat4(1.0f);
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 m_scale = glm::vec3(1.0f);
	std::list<Ref<Transform>> m_children;
	Transform* m_pParent = nullptr;
	mutable bool m_bDirty = false;
};
} // namespace le
