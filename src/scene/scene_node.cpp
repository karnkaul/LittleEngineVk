#include <core/utils/debug.hpp>
#include <dens/registry.hpp>
#include <engine/scene/scene_node.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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
} // namespace

glm::quat SceneTransform::worldOrientation(glm::mat4 const& mat) noexcept {
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(mat, scl, orn, pos, skw, psp);
	return glm::conjugate(orn);
}

glm::vec3 SceneTransform::worldScale(glm::mat4 const& mat) noexcept {
	glm::vec3 pos;
	glm::quat orn;
	glm::vec3 scl;
	glm::vec3 skw;
	glm::vec4 psp;
	glm::decompose(mat, scl, orn, pos, skw, psp);
	return scl;
}

bool SceneNode::parent(dens::registry const& registry, dens::entity parent) {
	if (auto node = registry.find<SceneNode>(parent)) {
		m_parent = parent;
		if (!node->hasNode(m_entity)) { node->m_nodes.push_back(m_entity); }
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
		refreshEntity(registry, m_parent);
		std::erase_if(m_nodes, [&registry](dens::entity e) { return !registry.attached<SceneNode>(e); });
	}
}

bool SceneNode::isotropic(dens::registry const& registry) const {
	if (!TSceneMatrix<SceneNode>::isotropic()) { return false; }
	if (auto e = registry.find<SceneNode>(m_parent)) { return e->isotropic(registry); }
	return true;
}

glm::mat4 SceneNode::model(dens::registry const& registry) const {
	auto ret = matrix();
	if (auto node = registry.find<SceneNode>(m_parent)) { return node->model(registry) * ret; }
	return ret;
}

glm::mat4 SceneNode::normalModel(dens::registry const& registry) const {
	return isotropic(registry) ? matrix() : glm::mat4(glm::inverse(glm::transpose(glm::mat3(matrix()))));
}

bool SceneNode::hasNode(dens::entity entity) const noexcept { return std::find(m_nodes.begin(), m_nodes.end(), entity) != m_nodes.end(); }
} // namespace le
