#pragma once
#include <core/span.hpp>
#include <core/utils/dirty_flag.hpp>
#include <dens/entity.hpp>
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

class SceneNode : public TSceneMatrix<SceneNode> {
  public:
	SceneNode(dens::entity entity = {}) noexcept : m_entity(entity) {}

	bool parent(dens::registry const& registry, dens::entity parent);
	SceneNode* parent(dens::registry const& registry) const;
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

inline SceneTransform const SceneTransform::identity = {};

// impl

inline glm::mat4 SceneTransform::matrix() const noexcept {
	static constexpr auto base = glm::mat4(1.0f);
	auto const t = glm::translate(base, position);
	auto const r = glm::toMat4(orientation);
	auto const s = glm::scale(base, scale);
	return t * r * s;
}
} // namespace le
