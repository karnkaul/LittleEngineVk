#pragma once
#include <list>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "std_types.hpp"

namespace le
{
///
/// \brief Class for user-friendly and cached 4x4 matrix transformation with support for parenting
///
class Transform final
{
private:
	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	mutable glm::mat4 m_normalMat = glm::mat4(1.0f);
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 m_scale = glm::vec3(1.0f);
	std::list<Transform*> m_children;
	Transform* m_pParent = nullptr;
	mutable bool m_bDirty = false;

public:
	///
	/// \brief Identity transform (default constructed)
	///
	static Transform const s_identity;

public:
	Transform();
	Transform(Transform&&);
	Transform& operator=(Transform&&);
	~Transform();

public:
	Transform& setPosition(glm::vec3 const& position);
	Transform& setOrientation(glm::quat const& orientation);
	Transform& setScale(f32 scale);
	Transform& setScale(glm::vec3 const& scale);
	///
	/// \brief Set `pParent` as the parent of this transform
	///
	/// Pass `nullptr` to unset
	///
	Transform& setParent(Transform* pParent);

	///
	/// \brief Obtain parent (if any)
	///
	Transform const* parent() const;
	///
	/// \brief Obtain parent (if any)
	///
	Transform* parent();

	glm::vec3 position() const;
	glm::quat orientation() const;
	glm::vec3 scale() const;
	///
	/// \brief Check if scale is uniform across all axes
	///
	bool isIsotropic() const;

	///
	/// \brief Obtain final position (after transformations from parent hierarchy)
	///
	glm::vec3 worldPosition() const;
	///
	/// \brief Obtain final orientation (after transformations from parent hierarchy)
	///
	glm::quat worldOrientation() const;
	///
	/// \brief Obtain final scale (after transformations from parent hierarchy)
	///
	glm::vec3 worldScale() const;

	///
	/// \brief Obtain transformation matrix (recompute if stale)
	///
	glm::mat4 model() const;
	///
	/// \brief Obtain normal matrix (recompute if stale)
	///
	glm::mat4 normalModel() const;

	///
	/// \brief Check if world transform is up to date
	///
	/// Returns `false` if any `Transform` in the hierarcy is stale
	///
	bool isUpToDate() const;
	///
	/// \brief Recompute transformation matrices if stale
	///
	void updateMats() const;
};
} // namespace le
