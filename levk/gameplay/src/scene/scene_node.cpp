#include <dens/registry.hpp>
#include <levk/core/utils/debug.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/gameplay/scene/scene_node.hpp>
#include <algorithm>

namespace le {
namespace {
bool refreshEntity(dens::registry const& registry, dens::entity& out) {
	if (!registry.attached<SceneNode>(out)) {
		out = {};
		return false;
	}
	return true;
}

bool find(std::span<dens::entity const> nodes, dens::entity node) noexcept { return std::find(nodes.begin(), nodes.end(), node) != nodes.end(); }
} // namespace

bool SceneNode::valid(dens::registry const& registry) const { return registry.all_attached<Transform, SceneNode>(m_entity); }

Transform& SceneNode::transform(dens::registry const& registry) const {
	EXPECT(valid(registry));
	return registry.get<Transform>(m_entity);
}

bool SceneNode::parent(dens::registry const& registry, dens::entity parent) {
	if (auto node = registry.find<SceneNode>(parent)) {
		if (auto p = registry.find<SceneNode>(m_parent)) { std::erase(p->m_nodes, m_entity); }
		m_parent = parent;
		if (!find(node->m_nodes, m_entity)) { node->m_nodes.push_back(m_entity); }
		return true;
	}
	return false;
}

SceneNode* SceneNode::parent(dens::registry const& registry) const { return registry.find<SceneNode>(m_parent); }

void SceneNode::clean(dens::registry const& registry) {
	if (!refreshEntity(registry, m_entity)) {
		m_parent = {};
		m_nodes.clear();
	} else {
		std::erase_if(m_nodes, [&registry](dens::entity e) { return !registry.attached<SceneNode>(e); });
	}
}

bool SceneNode::isotropic(dens::registry const& registry) const {
	if (!transform(registry).isotropic()) { return false; }
	if (auto e = registry.find<SceneNode>(m_parent)) { return e->isotropic(registry); }
	return true;
}

glm::mat4 SceneNode::model(dens::registry const& registry) const {
	auto ret = transform(registry).matrix();
	if (auto node = registry.find<SceneNode>(m_parent)) { return node->model(registry) * ret; }
	return ret;
}

glm::mat4 SceneNode::normalModel(dens::registry const& registry) const {
	return isotropic(registry) ? transform(registry).matrix() : glm::mat4(glm::inverse(glm::transpose(glm::mat3(transform(registry).matrix()))));
}
} // namespace le
