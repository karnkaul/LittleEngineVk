#include <algorithm>
#include <engine/gui/tree.hpp>
#include <engine/input/space.hpp>
#include <engine/render/viewport.hpp>

namespace le::gui {
DrawScissor scissor(input::Space const& space, glm::vec2 centre, glm::vec2 halfSize, bool normalised) noexcept {
	return {space.project(centre - halfSize, normalised) * space.world.density, space.project(centre + halfSize, normalised) * space.world.density};
}

void TreeNode::update(input::Space const& space) {
	m_rect.adjust(m_parent->m_rect);
	m_scissor = scissor(space);
	onUpdate(space);
	for (auto& node : m_ts) { node->update(space); }
}
} // namespace le::gui
