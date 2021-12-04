#include <engine/assets/asset_store.hpp>
#include <engine/engine.hpp>
#include <engine/render/list_drawer.hpp>

namespace le {
void ListDrawer::beginPass(PipelineFactory& pf, vk::RenderPass rp) {
	buildDrawLists(pf, rp);
	auto const cache = DescriptorHelper::Cache::make(Services::get<AssetStore>());
	ENSURE(m_drawLists.size() == m_lists.size(), "Invariant violated");
	[[maybe_unused]] std::size_t idx = 0;
	for (auto const& list : m_lists) {
		ENSURE(list.drawables.size() == m_drawLists[idx++].drawables.size(), "Invariant violated");
		writeSets(DescriptorMap(&cache, list.pipeline.shaderInput), list);
	}
}

void ListDrawer::draw(graphics::CommandBuffer cb) {
	std::unordered_set<graphics::ShaderInput*> pipes;
	ENSURE(m_drawLists.size() == m_lists.size(), "Invariant violated");
	[[maybe_unused]] std::size_t idx = 0;
	for (auto const& list : m_lists) {
		cb.m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, list.pipeline.pipeline);
		pipes.insert(list.pipeline.shaderInput);
		ENSURE(list.drawables.size() == m_drawLists[idx++].drawables.size(), "Invariant violated");
		draw(DescriptorBinder(list.pipeline.layout, list.pipeline.shaderInput, cb), list, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
	m_drawLists.clear();
	m_lists.clear();
	Engine::drawImgui(cb);
}

void ListDrawer::buildLists(graphics::PipelineFactory& pf, vk::RenderPass rp) {
	m_lists.clear();
	m_lists.reserve(m_drawLists.size());
	for (auto& list : m_drawLists) {
		ENSURE(list.group.state, "Invalid draw list");
		auto pipe = pf.get(*list.group.state, rp);
		m_lists.push_back({list.drawables, pipe});
	}
}
} // namespace le
