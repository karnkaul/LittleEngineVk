#include <algorithm>
#include <engine/gui/node.hpp>
#include <engine/input/space.hpp>
#include <engine/render/viewport.hpp>

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

void Root::update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset) {
	input::Space const space(fbSize, wSize, view);
	for (auto& node : m_nodes) {
		node->Node::update(space, fbSize, offset);
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

void Node::update(input::Space const& space, glm::vec2 extent, glm::vec2 origin) {
	m_origin = origin + (m_local.norm * extent * 0.5f) + m_local.offset;
	glm::vec2 const hs = {m_size.x * 0.5f, -m_size.y * 0.5f};
	m_scissor.lt = space.screen(m_origin - hs);
	m_scissor.rb = space.screen(m_origin + hs);
	onUpdate();
	for (auto& node : m_nodes) {
		node->Node::update(space, m_size, m_origin);
	}
}
} // namespace le::gui
