#include <levk/engine/assets/asset_store.hpp>
#include <levk/gameplay/scene/draw_list_gen.hpp>
#include <levk/gameplay/scene/list_renderer.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <unordered_set>

namespace le {
namespace {
constexpr EnumArray<PolygonMode, vk::PolygonMode> polygonModes = {vk::PolygonMode::eFill, vk::PolygonMode::eLine, vk::PolygonMode::ePoint};
constexpr EnumArray<Topology, vk::PrimitiveTopology> topologies = {
	vk::PrimitiveTopology::ePointList,	  vk::PrimitiveTopology::eLineList,		vk::PrimitiveTopology::eLineStrip,
	vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleFan,
};
} // namespace

void ListRenderer::add(DrawableMap& out_map, RenderPipeline const& rp, glm::mat4 const& model, MeshView const& mesh, std::optional<DrawScissor> scissor) {
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

void ListRenderer::fill(DrawableMap& out_map, AssetStore const& store, dens::registry const& registry) {
	DrawListGen{}(out_map, store, registry);
	DebugDrawListGen{}(out_map, store, registry);
}

void ListRenderer::render(RenderPass& out_rp, AssetStore const& store, DrawableMap map) {
	EXPECT(!out_rp.commandBuffers().empty());
	if (out_rp.commandBuffers().empty()) { return; }
	std::vector<DrawList> drawLists;
	drawLists.reserve(map.size());
	for (auto& [rpipe, list] : map) {
		if (auto pipe = out_rp.pipelineFactory().get(pipelineSpec(rpipe), out_rp.renderPass()); pipe.valid()) {
			drawLists.push_back(DrawList{{}, std::move(list), pipe, s64(rpipe.layer.order)});
		}
	}
	auto const cache = DescriptorHelper::Cache::make(&store);
	std::unordered_set<graphics::ShaderInput*> pipes;
	auto const& cb = out_rp.commandBuffers().front();
	cb.setViewportScissor(out_rp.viewport(), out_rp.scissor());
	std::sort(drawLists.begin(), drawLists.end());
	for (auto const& list : drawLists) {
		EXPECT(list.pipeline.valid());
		cb.m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, list.pipeline.pipeline);
		pipes.insert(list.pipeline.shaderInput);
		writeSets(DescriptorMap(&cache, list.pipeline.shaderInput), list);
		draw(DescriptorBinder(list.pipeline.layout, list.pipeline.shaderInput, cb), list, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
}

void ListRenderer::draw(DescriptorBinder bind, DrawList const& list, graphics::CommandBuffer const& cb) const {
	for (u32 const set : list.sets) { bind(set); }
	for (Drawable const& d : list.drawables) {
		for (u32 const set : d.sets) { bind(set); }
		if (d.scissor) { cb.setScissor(cast(*d.scissor)); }
		for (MeshObj const& mesh : d.mesh.mesh.meshViews()) {
			EXPECT(mesh.primitive);
			if (mesh.primitive) {
				for (u32 const set : d.mesh.sets) { bind(set); }
				mesh.primitive->draw(cb);
			}
		}
	}
}

void ListRenderer2::add(RenderMap& out_map, RenderPipeline const& rp, glm::mat4 const& mat, Span<Primitive const> prims, std::optional<DrawScissor> scissor) {
	out_map[rp].push(prims, mat, cast(scissor));
}

graphics::PipelineSpec ListRenderer2::pipelineSpec(RenderPipeline const& rp) {
	graphics::ShaderSpec ss;
	for (auto const& uri : rp.shaderURIs) { ss.moduleURIs.push_back(uri); }
	graphics::PipelineSpec ret = graphics::PipelineFactory::spec(ss, rp.layer.flags);
	ret.fixedState.lineWidth = rp.layer.lineWidth;
	ret.fixedState.mode = polygonModes[rp.layer.mode];
	ret.fixedState.topology = topologies[rp.layer.topology];
	return ret;
}

void ListRenderer2::fill(RenderMap& out_map, AssetStore const& store, dens::registry const& registry) {
	DrawListGen2{}(out_map, store, registry);
	// DebugDrawListGen{}(out_map, store, registry);
}

void ListRenderer2::render(RenderPass& out_rp, AssetStore const& store, RenderMap map) {
	EXPECT(!out_rp.commandBuffers().empty());
	if (out_rp.commandBuffers().empty()) { return; }
	std::vector<RenderList> drawLists;
	drawLists.reserve(map.size());
	for (auto& [rpipe, list] : map) {
		if (auto pipe = out_rp.pipelineFactory().get(pipelineSpec(rpipe), out_rp.renderPass()); pipe.valid()) {
			drawLists.push_back(RenderList{pipe, std::move(list), rpipe.layer.order});
		}
	}
	auto const cache = DescriptorHelper::Cache::make(&store);
	std::unordered_set<graphics::ShaderInput*> pipes;
	auto const& cb = out_rp.commandBuffers().front();
	m_scissor = out_rp.scissor();
	cb.setViewportScissor(out_rp.viewport(), out_rp.scissor());
	std::sort(drawLists.begin(), drawLists.end());
	for (auto const& list : drawLists) {
		EXPECT(list.pipeline.valid());
		cb.m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, list.pipeline.pipeline);
		pipes.insert(list.pipeline.shaderInput);
		writeSets(DescriptorMap(&cache, list.pipeline.shaderInput), list.drawList);
		draw(DescriptorBinder(list.pipeline.layout, list.pipeline.shaderInput, cb), list.drawList, cb);
	}
	for (auto pipe : pipes) { pipe->swap(); }
}
} // namespace le
