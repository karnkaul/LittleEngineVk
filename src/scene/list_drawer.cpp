#include <dumb_ecf/types.hpp>
#include <engine/scene/list_drawer.hpp>
#include <graphics/render/pipeline.hpp>

namespace le {
void ListDrawer::draw(graphics::CommandBuffer cb) {
	std::unordered_set<graphics::Pipeline*> pipes;
	ENSURE(m_drawLists.size() == m_lists.size(), "Invariant violated");
	[[maybe_unused]] std::size_t idx = 0;
	for (auto const& list : m_lists) {
		pipes.insert(list.pipeline);
		cb.bindPipe(*list.pipeline, list.variant);
		ENSURE(list.drawables.size() == m_drawLists[idx++].drawables.size(), "Invariant violated");
		draw(list, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
	m_drawLists.clear();
	m_lists.clear();
}
} // namespace le
