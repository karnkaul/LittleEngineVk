#include <algorithm>
#include <engine/gui/node.hpp>

namespace le::gui {
namespace {
Node* tryLeafHit(Node& root, glm::vec2 point) noexcept {
	for (auto& n : root.m_nodes) {
		if (auto ret = tryLeafHit(*n, point)) {
			return ret;
		}
	}
	return root.hit(point) ? &root : nullptr;
}
} // namespace

bool Root::pop(Node& node) noexcept {
	auto const it = std::find_if(m_nodes.begin(), m_nodes.end(), [&node](auto const& n) { return &node == n.get(); });
	if (it != m_nodes.end()) {
		m_nodes.erase(it);
		return true;
	}
	return false;
}

void Root::update(glm::vec2 space, glm::vec2 offset) {
	for (auto& node : m_nodes) {
		node->Node::update(space, offset);
	}
}

Node* Root::leafHit(glm::vec2 point) const noexcept {
	for (auto& n : m_nodes) {
		if (auto ret = tryLeafHit(*n, point)) {
			return ret;
		}
	}
	return nullptr;
}

void Node::update(glm::vec2 extent, glm::vec2 origin) {
	m_origin = origin + (m_local.norm * extent * 0.5f) + m_local.offset;
	onUpdate();
	for (auto& node : m_nodes) {
		node->Node::update(m_size, m_origin);
	}
}
} // namespace le::gui
