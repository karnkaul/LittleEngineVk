#pragma once
#include <list>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "std_types.hpp"

namespace le
{
class Transform final
{
private:
	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 m_scale = glm::vec3(1.0f);
	std::list<Transform*> m_children;
	Transform* m_pParent = nullptr;
	mutable bool m_bDirty = false;

public:
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
	Transform& setParent(Transform* pParent);

	Transform const* parent() const;
	Transform* parent();

	glm::vec3 position() const;
	glm::quat orientation() const;
	glm::vec3 scale() const;
	bool isIsotropic() const;

	glm::vec3 worldPosition() const;
	glm::quat worldOrientation() const;
	glm::vec3 worldScale() const;

	glm::mat4 model() const;
	glm::mat4 normalModel() const;
};
} // namespace le
