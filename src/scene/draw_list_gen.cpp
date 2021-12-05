#include <dens/registry.hpp>
#include <engine/ecs/components/trigger.hpp>
#include <engine/gui/tree.hpp>
#include <engine/gui/view.hpp>
#include <engine/render/mesh_provider.hpp>
#include <engine/scene/draw_list_gen.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
DrawScissor cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}, true}; }

void addNodes(ListDrawer::GroupMap& map, DrawGroupProvider const& group, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active && group.active()) {
			if (auto mesh = node->mesh(); !mesh.empty()) {
				DrawScissor const rect = cast(graphics::utils::scissor(node->m_scissor));
				ListDrawer::add(map, group.group(), node->model(), mesh, rect);
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, group, *node); }
	}
}
} // namespace

void DrawListGen::operator()(ListDrawer::GroupMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	auto modelMat = [&registry](dens::entity e) {
		if (auto n = registry.find<SceneNode>(e)) {
			return n->model(registry);
		} else if (auto t = registry.find<Transform>(e)) {
			return t->matrix();
		}
		return glm::mat4(1.0f);
	};
	for (auto [e, c] : registry.view<DrawGroupProvider, DynamicMesh>(exclude)) {
		auto& [group, dm] = c;
		if (group.active()) { ListDrawer::add(map, group.group(), modelMat(e), dm.mesh()); }
	}
	for (auto [e, c] : registry.view<DrawGroupProvider, MeshProvider>(exclude)) {
		auto& [group, provider] = c;
		if (group.active()) { ListDrawer::add(map, group.group(), modelMat(e), provider.mesh()); }
	}
	for (auto& [_, c] : registry.view<DrawGroupProvider, gui::ViewStack>()) {
		auto& [gr, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, gr, *view); }
	}
}

void DebugDrawListGen::operator()(ListDrawer::GroupMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	if (populate_v) {
		for (auto [_, c] : registry.view<DrawGroupProvider, physics::Trigger::Debug>(exclude)) {
			auto& [group, physics] = c;
			if (group.active()) {
				if (auto drawables = physics.drawables(registry); !drawables.empty()) {
					std::move(drawables.begin(), drawables.end(), std::back_inserter(map[group.group()]));
				}
			}
		}
	}
}
} // namespace le
