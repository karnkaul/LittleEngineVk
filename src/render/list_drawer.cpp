#include <engine/assets/asset_store.hpp>
#include <engine/engine.hpp>
#include <engine/render/list_drawer.hpp>
#include <engine/scene/draw_list_gen.hpp>
#include <graphics/mesh_primitive.hpp>
#include <unordered_set>

namespace le {
void ListDrawer::add(LayerMap& out_map, RenderLayer const& layer, glm::mat4 const& model, MeshView const& mesh, DrawScissor scissor) {
	if (layer.state && !mesh.empty()) { out_map[layer].push_back({{}, model, {{}, mesh}, scissor}); }
}

void ListDrawer::beginPass(PipelineFactory& pf, vk::RenderPass rp) {
	LayerMap map;
	populate(map);
	m_drawLists.clear();
	m_drawLists.reserve(map.size());
	for (auto& [layer, list] : map) {
		if (layer.state) {
			if (auto pipe = pf.get(*layer.state, rp); pipe.valid()) { m_drawLists.push_back(DrawList{{}, std::move(list), pipe, layer.order}); }
		}
	}
	std::sort(m_drawLists.begin(), m_drawLists.end());
	auto const cache = DescriptorHelper::Cache::make(Services::get<AssetStore>());
	for (auto const& list : m_drawLists) { writeSets(DescriptorMap(&cache, list.pipeline.shaderInput), list); }
}

void ListDrawer::fill(LayerMap& out_map, dens::registry const& registry) {
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

void ListDrawer::draw(DescriptorBinder bind, DrawList const& list, graphics::CommandBuffer cb) const {
	for (u32 const set : list.sets) { bind(set); }
	for (Drawable const& d : list.drawables) {
		for (u32 const set : d.sets) { bind(set); }
		if (d.scissor.set) { cb.setScissor(cast(d.scissor)); }
		for (MeshObj const& mesh : d.mesh.mesh.meshViews()) {
			for (u32 const set : d.mesh.sets) { bind(set); }
			mesh.primitive->draw(cb);
		}
	}
}
} // namespace le
