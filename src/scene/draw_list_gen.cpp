#include <dens/registry.hpp>
#include <engine/ecs/components/trigger.hpp>
#include <engine/gui/tree.hpp>
#include <engine/gui/view.hpp>
#include <engine/scene/draw_list_gen.hpp>
#include <engine/scene/mesh_provider.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
DrawScissor cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}, true}; }

void addNodes(ListDrawer::LayerMap& map, RenderPipeProvider const& rp, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active && rp.ready()) {
			if (auto mesh = node->mesh(); !mesh.empty()) {
				DrawScissor const rect = cast(graphics::utils::scissor(node->m_scissor));
				ListDrawer::add(map, rp.get(), node->model(), mesh, rect);
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, rp, *node); }
	}
}
} // namespace

void DrawListGen::operator()(ListDrawer::LayerMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	auto modelMat = [&registry](dens::entity e) {
		if (auto n = registry.find<SceneNode>(e)) {
			return n->model(registry);
		} else if (auto t = registry.find<Transform>(e)) {
			return t->matrix();
		}
		return glm::mat4(1.0f);
	};
	for (auto [e, c] : registry.view<RenderPipeProvider, DynamicMesh>(exclude)) {
		auto& [rl, dm] = c;
		if (rl.ready()) { ListDrawer::add(map, rl.get(), modelMat(e), dm.mesh()); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, MeshProvider>(exclude)) {
		auto& [rl, provider] = c;
		if (rl.ready()) { ListDrawer::add(map, rl.get(), modelMat(e), provider.mesh()); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, MeshView>(exclude)) {
		auto& [rl, view] = c;
		if (rl.ready()) { ListDrawer::add(map, rl.get(), modelMat(e), view); }
	}
	for (auto& [_, c] : registry.view<RenderPipeProvider, gui::ViewStack>()) {
		auto& [rp, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, rp, *view); }
	}
}

void DebugDrawListGen::operator()(ListDrawer::LayerMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	if (populate_v) {
		for (auto [_, c] : registry.view<RenderPipeProvider, physics::Trigger::Debug>(exclude)) {
			auto& [rl, physics] = c;
			if (rl.ready()) {
				if (auto drawables = physics.drawables(registry); !drawables.empty()) {
					std::move(drawables.begin(), drawables.end(), std::back_inserter(map[rl.get()]));
				}
			}
		}
	}
}
} // namespace le
