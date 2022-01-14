#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/render/list_renderer.hpp>
#include <levk/engine/scene/draw_list_gen.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <unordered_set>

#include <core/log.hpp>

namespace le {
namespace {
constexpr EnumArray<PolygonMode, vk::PolygonMode> polygonModes = {vk::PolygonMode::eFill, vk::PolygonMode::eLine, vk::PolygonMode::ePoint};
constexpr EnumArray<Topology, vk::PrimitiveTopology> topologies = {
	vk::PrimitiveTopology::ePointList,	  vk::PrimitiveTopology::eLineList,		vk::PrimitiveTopology::eLineStrip,
	vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleFan,
};
} // namespace

void ListRenderer::add(DrawableMap& out_map, RenderPipeline const& rp, glm::mat4 const& model, MeshView const& mesh, DrawScissor scissor) {
	if (!mesh.empty()) { out_map[rp].push_back({{}, model, {{}, mesh}, scissor}); }
}

graphics::PipelineSpec ListRenderer::pipelineSpec(RenderPipeline const& rp) {
	graphics::ShaderSpec ss;
	for (auto const& uri : rp.shaderURIs) { ss.moduleURIs.push_back(uri); }
	graphics::PipelineSpec ret = graphics::PipelineFactory::spec(ss, rp.layer.flags);
	ret.fixedState.lineWidth = rp.layer.lineWidth;
	ret.fixedState.mode = polygonModes[rp.layer.mode];
	ret.fixedState.topology = topologies[rp.layer.topology];
	return ret;
}

void ListRenderer::fill(DrawableMap& out_map, dens::registry const& registry) {
	DrawListGen{}(out_map, registry);
	DebugDrawListGen{}(out_map, registry);
}

void ListRenderer::render(RenderPass& out_rp, DrawableMap map) {
	EXPECT(!out_rp.commandBuffers().empty());
	if (out_rp.commandBuffers().empty()) { return; }
	std::vector<DrawList> drawLists;
	drawLists.reserve(map.size());
	for (auto& [rpipe, list] : map) {
		if (auto pipe = out_rp.pipelineFactory().get(pipelineSpec(rpipe), out_rp.renderPass()); pipe.valid()) {
			drawLists.push_back(DrawList{{}, std::move(list), pipe, rpipe.layer.order});
		}
	}
	std::sort(drawLists.begin(), drawLists.end());
	auto const cache = DescriptorHelper::Cache::make(Services::get<AssetStore>());
	for (auto const& list : drawLists) { writeSets(DescriptorMap(&cache, list.pipeline.shaderInput), list); }
	std::unordered_set<graphics::ShaderInput*> pipes;
	auto const& cb = out_rp.commandBuffers().front();
	cb.setViewportScissor(out_rp.viewport(), out_rp.scissor());
	for (auto const& list : drawLists) {
		EXPECT(list.pipeline.valid());
		cb.m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, list.pipeline.pipeline);
		pipes.insert(list.pipeline.shaderInput);
		draw(DescriptorBinder(list.pipeline.layout, list.pipeline.shaderInput, cb), list, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
	Engine::drawImgui(cb);
}

void ListRenderer::draw(DescriptorBinder bind, DrawList const& list, graphics::CommandBuffer const& cb) const {
	for (u32 const set : list.sets) { bind(set); }
	for (Drawable const& d : list.drawables) {
		for (u32 const set : d.sets) { bind(set); }
		if (d.scissor.set) { cb.setScissor(cast(d.scissor)); }
		for (MeshObj const& mesh : d.mesh.mesh.meshViews()) {
			EXPECT(mesh.primitive);
			if (mesh.primitive) {
				for (u32 const set : d.mesh.sets) { bind(set); }
				mesh.primitive->draw(cb);
			}
		}
	}
}
} // namespace le
