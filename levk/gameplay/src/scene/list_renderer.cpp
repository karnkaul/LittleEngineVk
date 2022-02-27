#include <dens/registry.hpp>
#include <levk/engine/assets/asset_provider.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/render/no_draw.hpp>
#include <levk/engine/render/primitive_provider.hpp>
#include <levk/gameplay/ecs/components/trigger.hpp>
#include <levk/gameplay/gui/tree.hpp>
#include <levk/gameplay/gui/view.hpp>
#include <levk/gameplay/scene/list_renderer.hpp>
#include <levk/gameplay/scene/scene_node.hpp>
#include <levk/graphics/mesh.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/skybox.hpp>
#include <levk/graphics/utils/utils.hpp>
#include <unordered_set>

namespace le {
namespace {
constexpr EnumArray<PolygonMode, vk::PolygonMode> polygonModes = {vk::PolygonMode::eFill, vk::PolygonMode::eLine, vk::PolygonMode::ePoint};
constexpr EnumArray<Topology, vk::PrimitiveTopology> topologies = {
	vk::PrimitiveTopology::ePointList,	  vk::PrimitiveTopology::eLineList,		vk::PrimitiveTopology::eLineStrip,
	vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleList, vk::PrimitiveTopology::eTriangleFan,
};
} // namespace

graphics::PipelineSpec ListRenderer::pipelineSpec(RenderPipeline const& rp) {
	graphics::ShaderSpec ss;
	for (auto const& uri : rp.shaderURIs) { ss.moduleURIs.push_back(uri); }
	graphics::PipelineSpec ret = graphics::PipelineFactory::spec(ss, rp.layer.flags);
	ret.fixedState.lineWidth = rp.layer.lineWidth;
	ret.fixedState.mode = polygonModes[rp.layer.mode];
	ret.fixedState.topology = topologies[rp.layer.topology];
	return ret;
}

void ListRenderer::fill(RenderMap& out_map, AssetStore const& store, dens::registry const& registry) {
	DrawListGen{}(out_map, store, registry);
	DebugDrawListGen{}(out_map, store, registry);
}

void ListRenderer::render(RenderPass& out_rp, AssetStore const& store, RenderMap map) {
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

namespace {
void addNodes(ListRenderer::RenderMap& map, RenderPipeline const& rp, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active) { node->addDrawPrimitives(map[rp]); }
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, rp, *node); }
	}
}
} // namespace

void DrawListGen::operator()(ListRenderer::RenderMap& map, AssetStore const& store, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	auto modelMat = [&registry](dens::entity e) {
		if (auto n = registry.find<SceneNode>(e)) {
			return n->model(registry);
		} else if (auto t = registry.find<Transform>(e)) {
			return t->matrix();
		}
		return glm::mat4(1.0f);
	};
	for (auto [e, c] : registry.view<RenderPipeProvider, AssetProvider<graphics::Skybox>>(exclude)) {
		auto& [rp, skybox] = c;
		if (auto s = skybox.find(store); s && rp.ready(store)) { map[rp.get(store)].add(*s, modelMat(e)); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, AssetProvider<graphics::Mesh>>(exclude)) {
		auto& [rp, mesh] = c;
		if (auto m = mesh.find(store); m && rp.ready(store)) { map[rp.get(store)].add(*m, modelMat(e)); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, PrimitiveProvider>(exclude)) {
		auto& [rp, prim] = c;
		if (rp.ready(store)) { prim.addDrawPrimitives(store, map[rp.get(store)], modelMat(e)); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, PrimitiveGenerator>(exclude)) {
		auto& [rp, prim] = c;
		if (rp.ready(store)) { prim.addDrawPrimitives(map[rp.get(store)], modelMat(e)); }
	}
	for (auto [_, c] : registry.view<RenderPipeProvider, gui::ViewStack>(exclude)) {
		auto& [rp, stack] = c;
		if (auto r = rp.find(store)) {
			for (auto const& view : stack.views()) { addNodes(map, *r, *view); }
		}
	}
}

void DebugDrawListGen::operator()(ListRenderer::RenderMap& map, AssetStore const& store, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	if (populate_v) {
		for (auto [_, c] : registry.view<RenderPipeProvider, physics::Trigger::Debug>(exclude)) {
			auto& [rp, physics] = c;
			if (rp.ready(store)) { physics.addDrawPrimitives(store, registry, map[rp.get(store)]); }
		}
	}
}
} // namespace le
