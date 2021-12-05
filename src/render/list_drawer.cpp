#include <engine/assets/asset_store.hpp>
#include <engine/engine.hpp>
#include <engine/render/list_drawer.hpp>
#include <engine/scene/draw_list_gen.hpp>
#include <unordered_set>

namespace le {
void ListDrawer::add(GroupMap& out_map, DrawGroup const& group, glm::mat4 const& model, MeshView const& mesh, Rect2D scissor) {
	if (group.state && !mesh.empty()) { out_map[group].push_back({model, scissor, mesh}); }
}

void ListDrawer::beginPass(PipelineFactory& pf, vk::RenderPass rp) {
	GroupMap map;
	populate(map);
	m_drawLists.clear();
	m_drawLists.reserve(map.size());
	for (auto& [group, list] : map) {
		if (group.state) {
			if (auto pipe = pf.get(*group.state, rp); pipe.valid()) { m_drawLists.push_back(DrawList{pipe, std::move(list), group.order}); }
		}
	}
	std::sort(m_drawLists.begin(), m_drawLists.end());
	auto const cache = DescriptorHelper::Cache::make(Services::get<AssetStore>());
	for (auto const& list : m_drawLists) { writeSets(DescriptorMap(&cache, list.pipeline.shaderInput), list); }
}

void ListDrawer::fill(GroupMap& out_map, dens::registry const& registry) {
	DrawListGen{}(out_map, registry);
	DebugDrawListGen{}(out_map, registry);
}

void ListDrawer::draw(graphics::CommandBuffer cb) {
	std::unordered_set<graphics::ShaderInput*> pipes;
	for (auto const& list : m_drawLists) {
		EXPECT(list.pipeline.valid());
		cb.m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, list.pipeline.pipeline);
		pipes.insert(list.pipeline.shaderInput);
		draw(DescriptorBinder(list.pipeline.layout, list.pipeline.shaderInput, cb), list, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
	m_drawLists.clear();
	Engine::drawImgui(cb);
}
} // namespace le
