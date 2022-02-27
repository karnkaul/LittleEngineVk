#include <dens/registry.hpp>
#include <levk/engine/assets/asset_provider.hpp>
#include <levk/engine/render/mesh_view_provider.hpp>
#include <levk/gameplay/ecs/components/trigger.hpp>
#include <levk/gameplay/gui/tree.hpp>
#include <levk/gameplay/gui/view.hpp>
#include <levk/gameplay/scene/draw_list_gen.hpp>
#include <levk/gameplay/scene/scene_node.hpp>
#include <levk/graphics/mesh.hpp>
#include <levk/graphics/utils/utils.hpp>

#include <levk/engine/render/primitive_provider.hpp>
#include <levk/graphics/skybox.hpp>

namespace le {
namespace {
DrawScissor cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}}; }

void addNodes(ListRenderer::DrawableMap& map, RenderPipeProvider const& rp, AssetStore const& store, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active && rp.ready(store)) {
			if (auto mesh = node->mesh(); !mesh.empty()) {
				DrawScissor const rect = cast(graphics::utils::scissor(node->m_scissor));
				ListRenderer::add(map, rp.get(store), node->matrix(), mesh, rect);
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, rp, store, *node); }
	}
}

void addNodes(ListRenderer2::RenderMap& map, RenderPipeline const& rp, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active) { node->addDrawPrimitives(map[rp]); }
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, rp, *node); }
	}
}
} // namespace

void DrawListGen::operator()(ListRenderer::DrawableMap& map, AssetStore const& store, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	auto modelMat = [&registry](dens::entity e) {
		if (auto n = registry.find<SceneNode>(e)) {
			return n->model(registry);
		} else if (auto t = registry.find<Transform>(e)) {
			return t->matrix();
		}
		return glm::mat4(1.0f);
	};
	for (auto [e, c] : registry.view<RenderPipeProvider, DynamicMeshView>(exclude)) {
		auto& [rp, dm] = c;
		if (rp.ready(store)) { ListRenderer::add(map, rp.get(store), modelMat(e), dm.mesh()); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, MeshViewProvider>(exclude)) {
		auto& [rp, provider] = c;
		if (rp.ready(store)) { ListRenderer::add(map, rp.get(store), modelMat(e), provider.mesh()); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, MeshView>(exclude)) {
		auto& [rp, view] = c;
		if (rp.ready(store)) { ListRenderer::add(map, rp.get(store), modelMat(e), view); }
	}
	for (auto& [_, c] : registry.view<RenderPipeProvider, gui::ViewStack>(exclude)) {
		auto& [rp, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, rp, store, *view); }
	}
}

void DrawListGen2::operator()(ListRenderer2::RenderMap& map, AssetStore const& store, dens::registry const& registry) const {
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

void DebugDrawListGen::operator()(ListRenderer2::RenderMap& map, AssetStore const& store, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	if (populate_v) {
		for (auto [_, c] : registry.view<RenderPipeProvider, physics::Trigger::Debug>(exclude)) {
			auto& [rp, physics] = c;
			if (rp.ready(store)) { physics.addDrawPrimitives(store, registry, map[rp.get(store)]); }
		}
	}
}
} // namespace le
