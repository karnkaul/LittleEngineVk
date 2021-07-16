#pragma once
#include <dumb_ecf/types.hpp>
#include <engine/utils/ref_tree.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace le {
struct SceneTransform {
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	static SceneTransform const identity;

	glm::mat4 matrix() const noexcept;
};

class SceneNode : public utils::RefTreeNode<SceneNode> {
  public:
	using Root = typename RefTreeNode<SceneNode>::Root;

	SceneNode(not_null<Root*> root, decf::entity entity = {}, SceneTransform const& transform = {});

	///
	/// \brief Set (local) position
	///
	SceneNode& position(glm::vec3 const& position) noexcept;
	///
	/// \brief Set (local) orientation
	///
	SceneNode& orient(glm::quat const& orientation) noexcept;
	///
	/// \brief Rotate (local) orientation
	///
	SceneNode& rotate(f32 radians, glm::vec3 const& axis) noexcept;
	///
	/// \brief Set (local) scale
	///
	SceneNode& scale(f32 scale) noexcept;
	///
	/// \brief Set (local) scale
	///
	SceneNode& scale(glm::vec3 const& scale) noexcept;
	///
	/// \brief Reset to default state
	///
	SceneNode& reset(SceneTransform const& transform = {});

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
	SceneTransform const& transform() const noexcept;
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
	/// Returns `true` if any `SceneNode` in the hierarcy is stale
	///
	bool stale() const noexcept;
	///
	/// \brief Recompute transformation matrices if stale
	///
	void refresh() const noexcept;

	decf::entity entity() const noexcept;
	void entity(decf::entity entity) noexcept;

  private:
	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	mutable glm::mat4 m_normalMat = glm::mat4(1.0f);
	SceneTransform m_transform;
	decf::entity m_entity;
	mutable bool m_dirty = false;
};

inline SceneTransform const SceneTransform::identity = {};

// impl

inline glm::mat4 SceneTransform::matrix() const noexcept {
	static constexpr auto base = glm::mat4(1.0f);
	auto const t = glm::translate(base, position);
	auto const r = glm::toMat4(orientation);
	auto const s = glm::scale(base, scale);
	return t * r * s;
}

inline SceneNode::SceneNode(not_null<Root*> parent, decf::entity entity, SceneTransform const& transform)
	: RefTreeNode(parent), m_transform(transform), m_entity(entity) {}

inline SceneNode& SceneNode::reset(SceneTransform const& transform) {
	m_transform = transform;
	refresh();
	return *this;
}

inline SceneNode& SceneNode::position(glm::vec3 const& position) noexcept {
	m_transform.position = position;
	m_dirty = true;
	return *this;
}
inline SceneNode& SceneNode::orient(glm::quat const& orientation) noexcept {
	m_transform.orientation = orientation;
	m_dirty = true;
	return *this;
}
inline SceneNode& SceneNode::rotate(f32 radians, glm::vec3 const& axis) noexcept {
	m_transform.orientation = glm::rotate(m_transform.orientation, radians, axis);
	m_dirty = true;
	return *this;
}
inline SceneNode& SceneNode::scale(f32 scale) noexcept {
	m_transform.scale = {scale, scale, scale};
	m_dirty = true;
	return *this;
}
inline SceneNode& SceneNode::scale(glm::vec3 const& scale) noexcept {
	m_transform.scale = scale;
	m_dirty = true;
	return *this;
}

inline glm::vec3 const& SceneNode::position() const noexcept { return m_transform.position; }

inline glm::quat const& SceneNode::orientation() const noexcept { return m_transform.orientation; }

inline glm::vec3 const& SceneNode::scale() const noexcept { return m_transform.scale; }

inline SceneTransform const& SceneNode::transform() const noexcept { return m_transform; }

inline bool SceneNode::isotropic() const noexcept {
	return m_transform.scale.x == m_transform.scale.y && m_transform.scale.y == m_transform.scale.z &&
		   (m_parent->isRoot() || static_cast<SceneNode const*>(m_parent.get())->isotropic());
}

inline glm::vec3 SceneNode::worldPosition() const noexcept { return glm::vec3(model()[3]); }

inline glm::mat4 SceneNode::model() const noexcept {
	refresh();
	return m_parent->isRoot() ? m_mat : static_cast<SceneNode const*>(m_parent.get())->model() * m_mat;
}

inline glm::mat4 SceneNode::normalModel() const noexcept {
	refresh();
	return m_normalMat;
}

inline bool SceneNode::stale() const noexcept { return m_dirty || (m_parent->isRoot() ? false : static_cast<SceneNode const*>(m_parent.get())->stale()); }

inline void SceneNode::refresh() const noexcept {
	if (m_dirty) {
		m_mat = m_transform.matrix();
		m_normalMat = isotropic() ? m_mat : glm::mat4(glm::inverse(glm::transpose(glm::mat3(m_mat))));
		m_dirty = false;
	}
	return;
}

inline decf::entity SceneNode::entity() const noexcept { return m_entity; }

inline void SceneNode::entity(decf::entity entity) noexcept { m_entity = entity; }
} // namespace le
