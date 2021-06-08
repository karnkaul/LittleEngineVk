#include <engine/gui/view.hpp>
#include <engine/gui/widget.hpp>
#include <engine/input/space.hpp>

namespace le::gui {
namespace {
TreeNode* tryLeafHit(TreeNode& root, glm::vec2 point) noexcept {
	// dfs
	for (auto& n : root.nodes()) {
		if (auto ret = tryLeafHit(*n, point)) { return ret; }
	}
	return root.hit(point) ? &root : nullptr;
}
} // namespace

TreeNode* View::leafHit(glm::vec2 point) const noexcept {
	if (!destroyed()) {
		for (auto& n : nodes()) {
			if (auto ret = tryLeafHit(*n, point)) { return ret; }
		}
	}
	return nullptr;
}

void View::update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset) {
	if (!destroyed()) {
		input::Space const space(fbSize, wSize, view);
		m_rect.size = m_canvas.size(fbSize);
		m_rect.origin = m_canvas.centre(fbSize) + offset;
		onUpdate(space);
		for (auto& node : m_ts) { node->update(space); }
	}
}

void ViewStack::update(input::State const& state, Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset) {
	std::erase_if(m_ts, [](auto& stack) { return stack->destroyed(); });
	for (auto& v : m_ts) { v->update(view, fbSize, wSize, offset); }
	for (auto it = m_ts.rbegin(); it != m_ts.rend(); ++it) {
		auto& v = *it;
		for (auto& node : v->nodes()) {
			if (auto widget = dynamic_cast<Widget*>(node.get())) { widget->onInput(state); }
		}
		if (v->m_block == View::Block::eBlock) { break; }
	}
}
} // namespace le::gui
