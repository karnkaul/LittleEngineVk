#include <algorithm>
#include <engine/gui/node.hpp>
#include <engine/input/space.hpp>
#include <engine/render/viewport.hpp>

namespace le::gui {
namespace {
Node* tryLeafHit(Node& root, glm::vec2 point) noexcept {
	for (auto& n : root.m_nodes) {
		if (n->m_hitTest) {
			if (auto ret = tryLeafHit(*n, point)) { return ret; }
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

void Root::update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset) {
	input::Space const space(fbSize, wSize, view);
	m_size = fbSize;
	m_origin = offset;
	for (auto& node : m_nodes) { node->Node::update(space); }
}

Node* Root::leafHit(glm::vec2 point) const noexcept {
	for (auto& n : m_nodes) {
		if (auto ret = tryLeafHit(*n, point)) { return ret; }
	}
	return nullptr;
}

void Node::update(input::Space const& space) {
	m_origin = m_parent->m_origin + (m_local.norm * m_parent->m_size) + m_local.offset;
	m_scissor.lt = space.screen({-0.5f, 0.5f}, true);
	m_scissor.rb = space.screen({0.5f, -0.5f}, true);
	onUpdate(space);
	for (auto& node : m_nodes) { node->Node::update(space); }
}
} // namespace le::gui
