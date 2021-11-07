#pragma once
#include <core/utils/dirty_flag.hpp>
#include <dens/entity.hpp>
#include <engine/utils/ref_tree.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace dens {
class registry;
}

namespace le {
struct SceneTransform {
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	static SceneTransform const identity;

	glm::mat4 matrix() const noexcept;
	bool isotropic() const noexcept { return scale.x == scale.y && scale.y == scale.z; }

	static glm::vec3 worldPosition(glm::mat4 const& mat) noexcept { return mat[3]; }
	static glm::quat worldOrientation(glm::mat4 const& mat) noexcept;
	static glm::vec3 worldScale(glm::mat4 const& mat) noexcept;
};

class SceneNode : public utils::RefTreeNode<SceneNode>, utils::DirtyFlag {
  public:
	using Root = typename RefTreeNode<SceneNode>::Root;

	struct Disable {};

	SceneNode(not_null<Root*> root, dens::entity entity = {}, SceneTransform const& transform = {});
	SceneNode(SceneNode&& rhs);
	SceneNode& operator=(SceneNode&&) = default;

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

	dens::entity entity() const noexcept;
	void entity(dens::entity entity) noexcept;

  private:
	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	mutable glm::mat4 m_normalMat = glm::mat4(1.0f);
	SceneTransform m_transform;
	dens::entity m_entity;
};

class SceneRoot {
  public:
	SceneRoot(dens::entity parent = {}) noexcept : m_parent(parent) {}

  protected:
	std::vector<dens::entity> m_nodes;
	dens::entity m_parent;
	dens::entity m_entity;
};

template <typename T>
class TSceneMatrix : utils::DirtyFlag {
  public:
	T& position(glm::vec3 const& position) noexcept { return (m_transform.position = position, makeDirty()); }
	T& orient(glm::quat const& orientation) noexcept { return (m_transform.orientation = orientation, makeDirty()); }
	T& rotate(f32 radians, glm::vec3 const& axis) noexcept { return orient(glm::rotate(m_transform.orientation, radians, axis)); }
	T& scale(f32 scale) noexcept { return this->scale({scale, scale, scale}); }
	T& scale(glm::vec3 const& scale) noexcept { return (m_transform.scale = scale, makeDirty()); }
	T& reset(SceneTransform const& transform = {}) { return (m_transform = transform, makeDirty()); }

	glm::vec3 const& position() const noexcept { return m_transform.position; }
	glm::quat const& orientation() const noexcept { return m_transform.orientation; }
	glm::vec3 const& scale() const noexcept { return m_transform.scale; }
	SceneTransform const& transform() const noexcept { return m_transform; }
	bool isotropic() const noexcept { return m_transform.isotropic(); }

	glm::mat4 matrix() const noexcept { return (refresh(), m_mat); }

	bool stale() const noexcept { return dirty(); }
	void refresh() const noexcept {
		if (stale()) { m_mat = m_transform.matrix(); }
	}

  private:
	T& makeDirty() noexcept { return (setDirty(), static_cast<T&>(*this)); }

	mutable glm::mat4 m_mat = glm::mat4(1.0f);
	SceneTransform m_transform;
};

class SceneNode2 : public TSceneMatrix<SceneNode2> {
  public:
	SceneNode2(dens::entity entity = {}) noexcept : m_entity(entity) {}

	bool parent(dens::registry const& registry, dens::entity parent);
	SceneNode2* parent(dens::registry const& registry) const;
	void clean(dens::registry const& registry);
	dens::entity entity() const noexcept { return m_entity; }
	Span<dens::entity const> nodes() const noexcept { return m_nodes; }

	bool isotropic(dens::registry const& registry) const;
	glm::mat4 model(dens::registry const& registry) const;
	glm::mat4 normalModel(dens::registry const& registry) const;

  private:
	bool hasNode(dens::entity node) const noexcept;

	std::vector<dens::entity> m_nodes;
	dens::entity m_parent;
	dens::entity m_entity;
};

using SceneRoot2 = std::vector<dens::entity>;

inline SceneTransform const SceneTransform::identity = {};

// impl

inline glm::mat4 SceneTransform::matrix() const noexcept {
	static constexpr auto base = glm::mat4(1.0f);
	auto const t = glm::translate(base, position);
	auto const r = glm::toMat4(orientation);
	auto const s = glm::scale(base, scale);
	return t * r * s;
}

inline SceneNode::SceneNode(not_null<Root*> parent, dens::entity entity, SceneTransform const& transform)
	: RefTreeNode(parent), m_transform(transform), m_entity(entity) {
	parent->addChild(this);
}

inline SceneNode::SceneNode(SceneNode&& rhs) : RefTreeNode(std::move(rhs)) { m_parent->addChild(this); }

inline SceneNode& SceneNode::reset(SceneTransform const& transform) {
	m_transform = transform;
	refresh();
	return *this;
}

inline SceneNode& SceneNode::position(glm::vec3 const& position) noexcept {
	m_transform.position = position;
	setDirty();
	return *this;
}
inline SceneNode& SceneNode::orient(glm::quat const& orientation) noexcept {
	m_transform.orientation = orientation;
	setDirty();
	return *this;
}
inline SceneNode& SceneNode::rotate(f32 radians, glm::vec3 const& axis) noexcept {
	m_transform.orientation = glm::rotate(m_transform.orientation, radians, axis);
	setDirty();
	return *this;
}
inline SceneNode& SceneNode::scale(f32 scale) noexcept {
	m_transform.scale = {scale, scale, scale};
	setDirty();
	return *this;
}
inline SceneNode& SceneNode::scale(glm::vec3 const& scale) noexcept {
	m_transform.scale = scale;
	setDirty();
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

inline bool SceneNode::stale() const noexcept { return dirty() || (m_parent->isRoot() ? false : static_cast<SceneNode const*>(m_parent.get())->stale()); }

inline void SceneNode::refresh() const noexcept {
	if (auto c = Clean(*this)) {
		m_mat = m_transform.matrix();
		m_normalMat = isotropic() ? m_mat : glm::mat4(glm::inverse(glm::transpose(glm::mat3(m_mat))));
	}
	return;
}

inline dens::entity SceneNode::entity() const noexcept { return m_entity; }

inline void SceneNode::entity(dens::entity entity) noexcept { m_entity = entity; }
} // namespace le
