#pragma once
#include <dens/entity.hpp>
#include <levk/core/span.hpp>
#include <levk/core/transform.hpp>

namespace dens {
class registry;
}

namespace le {
class SceneNode {
  public:
	SceneNode(dens::entity entity = {}) noexcept : m_entity(entity) {}

	bool valid(dens::registry const& registry) const;
	Transform& transform(dens::registry const& registry) const;
	bool parent(dens::registry const& registry, dens::entity parent);
	SceneNode* parent(dens::registry const& registry) const;
	void clean(dens::registry const& registry);
	dens::entity entity() const noexcept { return m_entity; }
	Span<dens::entity const> nodes() const noexcept { return m_nodes; }

	bool isotropic(dens::registry const& registry) const;
	glm::mat4 model(dens::registry const& registry) const;
	glm::mat4 normalModel(dens::registry const& registry) const;

  private:
	std::vector<dens::entity> m_nodes;
	dens::entity m_parent;
	dens::entity m_entity;
};
} // namespace le
