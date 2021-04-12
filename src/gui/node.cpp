#include <algorithm>
#include <engine/gui/node.hpp>

namespace le::gui {
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

void Node::update(glm::vec2 extent, glm::vec2 origin) {
	m_origin = origin + (m_local.norm * extent * 0.5f) + m_local.offset;
	onUpdate();
	for (auto& node : m_nodes) {
		node->Node::update(m_size, m_origin);
	}
}
} // namespace le::gui
