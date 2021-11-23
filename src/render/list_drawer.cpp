#include <engine/engine.hpp>
#include <engine/render/list_drawer.hpp>
#include <graphics/render/pipeline.hpp>

namespace le {
void ListDrawer::beginPass(PipelineFactory& pf, vk::RenderPass rp) {
	buildDrawLists(pf, rp);
	ENSURE(m_drawLists.size() == m_lists.size(), "Invariant violated");
	[[maybe_unused]] std::size_t idx = 0;
	for (auto const& list : m_lists) {
		ENSURE(list.drawables.size() == m_drawLists[idx++].drawables.size(), "Invariant violated");
		writeSets(DescriptorMap(&list.pipeline->shaderInput()), list);
	}
}

void ListDrawer::draw(graphics::CommandBuffer cb) {
	std::unordered_set<graphics::Pipeline*> pipes;
	ENSURE(m_drawLists.size() == m_lists.size(), "Invariant violated");
	[[maybe_unused]] std::size_t idx = 0;
	for (auto const& list : m_lists) {
		cb.bindPipe(*list.pipeline, list.variant);
		pipes.insert(list.pipeline);
		ENSURE(list.drawables.size() == m_drawLists[idx++].drawables.size(), "Invariant violated");
		draw(DescriptorBinder(list.pipeline->layout(), &list.pipeline->shaderInput(), cb), list, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
	m_drawLists.clear();
	m_lists.clear();
	Engine::drawImgui(cb);
}

void ListDrawer2::beginPass(PipelineFactory& pf, vk::RenderPass rp) {
	buildDrawLists(pf, rp);
	ENSURE(m_drawLists.size() == m_lists.size(), "Invariant violated");
	[[maybe_unused]] std::size_t idx = 0;
	for (auto const& list : m_lists) {
		ENSURE(list.drawables.size() == m_drawLists[idx++].drawables.size(), "Invariant violated");
		writeSets(DescriptorMap(list.pipeline.shaderInput), list);
	}
}

void ListDrawer2::draw(graphics::CommandBuffer cb) {
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

void ListDrawer2::buildLists(graphics::PipelineFactory& pf, vk::RenderPass rp) {
	m_lists.clear();
	m_lists.reserve(m_drawLists.size());
	for (auto& list : m_drawLists) {
		auto pipe = pf.get(list.group.state, rp);
		m_lists.push_back({list.drawables, pipe});
	}
}
} // namespace le
