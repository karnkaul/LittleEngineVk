#include <algorithm>
#include <engine/gui/tree.hpp>
#include <engine/input/space.hpp>
#include <engine/render/viewport.hpp>

namespace le::gui {
void TreeNode::update(input::Space const& space) {
	m_rect.adjust(m_parent->m_rect);
	m_scissor.lt = space.screen({-0.5f, 0.5f}, true);
	m_scissor.rb = space.screen({0.5f, -0.5f}, true);
	onUpdate(space);
	for (auto& node : m_ts) { node->update(space); }
}
} // namespace le::gui
