#include <algorithm>
#include <glm/gtx/matrix_decompose.hpp>
#include <core/assert.hpp>
#include <core/transform.hpp>

namespace le
{
Transform const Transform::s_identity;

Transform::Transform() = default;
Transform::~Transform()
{
	if (m_pParent)
	{
		m_pParent->m_children.remove(this);
	}
	for (auto pChild : m_children)
	{
		pChild->m_pParent = m_pParent;
		if (m_pParent)
		{
			m_pParent->m_children.push_back(pChild);
		}
		pChild->m_bDirty = true;
	}
}

Transform& Transform::setPosition(glm::vec3 const& position)
{
	m_position = position;
	m_bDirty = true;
	return *this;
}

Transform& Transform::setOrientation(glm::quat const& orientation)
{
	m_orientation = orientation;
	m_bDirty = true;
	return *this;
}

Transform& Transform::setScale(f32 scale)
{
	m_scale = {scale, scale, scale};
	m_bDirty = true;
	return *this;
}

Transform& Transform::setScale(glm::vec3 const& scale)
{
	m_scale = scale;
	m_bDirty = true;
	return *this;
}

Transform const* Transform::parent() const
{
	return m_pParent;
}

Transform* Transform::parent()
{
	return m_pParent;
}

Transform& Transform::setParent(Transform* pParent)
{
	ASSERT(pParent != this, "Setting parent to self!");
	if (pParent != this && m_pParent != pParent)
	{
		if (m_pParent)
		{
			m_pParent->m_children.remove(this);
		}
		m_pParent = pParent;
		if (m_pParent)
		{
			m_pParent->m_children.push_back(this);
		}
		m_bDirty = true;
	}
	return *this;
}

glm::vec3 const& Transform::position() const
{
	return m_position;
}

glm::quat const& Transform::orientation() const
{
	return m_orientation;
}

glm::vec3 const& Transform::scale() const
{
	return m_scale;
}

bool Transform::isIsotropic() const
{
	return m_scale.x == m_scale.y && m_scale.y == m_scale.z && (!m_pParent || m_pParent->isIsotropic());
}

glm::vec3 Transform::worldPosition() const
{
	return glm::vec3(model()[3]);
}

glm::quat Transform::worldOrientation() const
{
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(model(), scl, orn, pos, skw, psp);
	return glm::conjugate(orn);
}

glm::vec3 Transform::worldScale() const
{
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(model(), scl, orn, pos, skw, psp);
	return scl;
}

glm::mat4 Transform::model() const
{
	updateMats();
	return m_pParent ? m_pParent->model() * m_mat : m_mat;
}

glm::mat4 Transform::normalModel() const
{
	updateMats();
	return m_normalMat;
}

bool Transform::isUpToDate() const
{
	return !m_bDirty && m_pParent ? m_pParent->isUpToDate() : true;
}

void Transform::updateMats() const
{
	if (m_bDirty)
	{
		static auto const base = glm::mat4(1.0f);
		auto const t = glm::translate(base, m_position);
		auto const r = glm::toMat4(m_orientation);
		auto const s = glm::scale(base, m_scale);
		m_mat = t * r * s;
		m_normalMat = isIsotropic() ? m_mat : glm::mat4(glm::inverse(glm::transpose(glm::mat3(m_mat))));
		m_bDirty = false;
	}
	return;
}

std::list<Transform*> const& Transform::children() const
{
	return m_children;
}
} // namespace le
