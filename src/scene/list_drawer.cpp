#include <dumb_ecf/types.hpp>
#include <engine/scene/list_drawer.hpp>
#include <graphics/render/pipeline.hpp>

namespace le {
void ListDrawer::attach(decf::registry& registry, decf::entity entity, DrawLayer layer, Span<Primitive const> primitives) {
	DrawListFactory::attach(registry, entity, layer, primitives);
}

void ListDrawer::draw(graphics::CommandBuffer cb) const {
	std::unordered_set<graphics::Pipeline*> pipes;
	for (auto const& list : m_lists) {
		if (list.layer.pipeline) {
			pipes.insert(list.layer.pipeline);
			cb.bindPipe(*list.layer.pipeline);
			draw(list, cb);
		}
	}
	for (auto pipe : pipes) { pipe->swap(); }
}
} // namespace le
