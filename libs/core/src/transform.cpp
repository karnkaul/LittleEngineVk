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
		m_pParent->m_children.remove(*this);
	}
	for (Transform& child : m_children)
	{
		child.m_pParent = m_pParent;
		if (m_pParent)
		{
			m_pParent->m_children.push_back(child);
		}
		child.m_bDirty = true;
	}
}

Transform& Transform::position(glm::vec3 const& position) noexcept
{
	m_position = position;
	m_bDirty = true;
	return *this;
}

Transform& Transform::orient(glm::quat const& orientation) noexcept
{
	m_orientation = orientation;
	m_bDirty = true;
	return *this;
}

Transform& Transform::scale(f32 scale) noexcept
{
	m_scale = {scale, scale, scale};
	m_bDirty = true;
	return *this;
}

Transform& Transform::scale(glm::vec3 const& scale) noexcept
{
	m_scale = scale;
	m_bDirty = true;
	return *this;
}

Transform& Transform::parent(Transform* pParent)
{
	ASSERT(pParent != this, "Setting parent to self!");
	if (pParent != this && m_pParent != pParent)
	{
		if (m_pParent)
		{
			m_pParent->m_children.remove(*this);
		}
		m_pParent = pParent;
		if (m_pParent)
		{
			m_pParent->m_children.push_back(*this);
		}
		m_bDirty = true;
	}
	return *this;
}

void Transform::reset(bool bUnparent)
{
	if (bUnparent)
	{
		for (Transform& child : m_children)
		{
			child.m_pParent = m_pParent;
			if (m_pParent)
			{
				m_pParent->m_children.push_back(child);
			}
			child.m_bDirty = true;
		}
		m_children.clear();
		parent(nullptr);
	}
	m_position = {};
	m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	m_scale = glm::vec3(1.0f);
	updateMats();
}

Transform const* Transform::parent() const noexcept
{
	return m_pParent;
}

Transform* Transform::parent() noexcept
{
	return m_pParent;
}

glm::vec3 const& Transform::position() const noexcept
{
	return m_position;
}

glm::quat const& Transform::orientation() const noexcept
{
	return m_orientation;
}

glm::vec3 const& Transform::scale() const noexcept
{
	return m_scale;
}

bool Transform::isotropic() const noexcept
{
	return m_scale.x == m_scale.y && m_scale.y == m_scale.z && (!m_pParent || m_pParent->isotropic());
}

glm::vec3 Transform::worldPosition() const noexcept
{
	return glm::vec3(model()[3]);
}

glm::quat Transform::worldOrientation() const noexcept
{
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(model(), scl, orn, pos, skw, psp);
	return glm::conjugate(orn);
}

glm::vec3 Transform::worldScale() const noexcept
{
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(model(), scl, orn, pos, skw, psp);
	return scl;
}

glm::mat4 Transform::model() const noexcept
{
	updateMats();
	return m_pParent ? m_pParent->model() * m_mat : m_mat;
}

glm::mat4 Transform::normalModel() const noexcept
{
	updateMats();
	return m_normalMat;
}

bool Transform::stale() const noexcept
{
	return m_bDirty || (m_pParent ? m_pParent->stale() : false);
}

void Transform::updateMats() const noexcept
{
	if (m_bDirty)
	{
		static auto const base = glm::mat4(1.0f);
		auto const t = glm::translate(base, m_position);
		auto const r = glm::toMat4(m_orientation);
		auto const s = glm::scale(base, m_scale);
		m_mat = t * r * s;
		m_normalMat = isotropic() ? m_mat : glm::mat4(glm::inverse(glm::transpose(glm::mat3(m_mat))));
		m_bDirty = false;
	}
	return;
}

std::list<Ref<Transform>> const& Transform::children() const noexcept
{
	return m_children;
}
} // namespace le
