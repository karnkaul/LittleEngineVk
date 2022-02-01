#include <dens/registry.hpp>
#include <levk/engine/assets/asset_provider.hpp>
#include <levk/engine/gui/tree.hpp>
#include <levk/engine/gui/view.hpp>
#include <levk/engine/render/mesh_view_provider.hpp>
#include <levk/gameplay/ecs/components/trigger.hpp>
#include <levk/gameplay/scene/draw_list_gen.hpp>
#include <levk/gameplay/scene/scene_node.hpp>
#include <levk/graphics/utils/utils.hpp>

namespace le {
namespace {
DrawScissor cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}, true}; }

void addNodes(ListRenderer::DrawableMap& map, RenderPipeProvider const& rp, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active && rp.ready()) {
			if (auto mesh = node->mesh(); !mesh.empty()) {
				DrawScissor const rect = cast(graphics::utils::scissor(node->m_scissor));
				ListRenderer::add(map, rp.get(), node->model(), mesh, rect);
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, rp, *node); }
	}
}
} // namespace

void DrawListGen::operator()(ListRenderer::DrawableMap& map, dens::registry const& registry) const {
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
		if (rl.ready()) { ListRenderer::add(map, rl.get(), modelMat(e), dm.mesh()); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, MeshProvider>(exclude)) {
		auto& [rl, provider] = c;
		if (rl.ready()) { ListRenderer::add(map, rl.get(), modelMat(e), provider.mesh()); }
	}
	for (auto [e, c] : registry.view<RenderPipeProvider, MeshView>(exclude)) {
		auto& [rl, view] = c;
		if (rl.ready()) { ListRenderer::add(map, rl.get(), modelMat(e), view); }
	}
	for (auto& [_, c] : registry.view<RenderPipeProvider, gui::ViewStack>(exclude)) {
		auto& [rp, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, rp, *view); }
	}
}

void DebugDrawListGen::operator()(ListRenderer::DrawableMap& map, dens::registry const& registry) const {
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
