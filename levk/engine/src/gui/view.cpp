#include <levk/engine/gui/view.hpp>
#include <levk/engine/gui/widget.hpp>
#include <levk/engine/input/space.hpp>

namespace le::gui {
namespace {
TreeNode* tryLeafHit(TreeNode& root, glm::vec2 point) noexcept {
	// dfs
	for (auto& n : root.nodes()) {
		if (auto ret = tryLeafHit(*n, point)) { return ret; }
	}
	return root.hit(point) ? &root : nullptr;
}

bool tryPop(TreeRoot& root, TreeNode const* node) noexcept {
	if (root.pop(node)) { return true; }
	for (auto& n : root.nodes()) {
		if (tryPop(*n, node)) { return true; }
	}
	return false;
}
} // namespace

bool View::popRecurse(TreeNode const* node) noexcept { return tryPop(*this, node); }

TreeNode* View::leafHit(glm::vec2 point) const noexcept {
	if (!destroyed()) {
		for (auto& n : nodes()) {
			if (auto ret = tryLeafHit(*n, point)) { return ret; }
		}
	}
	return nullptr;
}

void View::update(input::Frame const& frame, glm::vec2 offset) {
	if (!destroyed()) {
		m_rect.size = m_canvas.size(frame.space.display.swapchain);
		m_rect.origin = m_canvas.centre(frame.space.display.swapchain) + offset;
		onUpdate(frame);
		for (auto& node : m_ts) { node->update(frame.space); }
	}
}

void ViewStack::update(input::Frame const& frame, glm::vec2 offset) {
	std::erase_if(m_ts, [](auto& stack) { return stack->destroyed(); });
	for (auto& v : m_ts) { v->update(frame, offset); }
	for (auto it = m_ts.rbegin(); it != m_ts.rend(); ++it) {
		auto& v = *it;
		v->forEachNode<Widget>(&Widget::onInput, frame.state);
		if (v->m_block == View::Block::eBlock) { break; }
	}
}
} // namespace le::gui
