#include <dens/registry.hpp>
#include <engine/ecs/components/trigger.hpp>
#include <engine/gui/tree.hpp>
#include <engine/gui/view.hpp>
#include <engine/render/mesh_provider.hpp>
#include <engine/render/prop_provider.hpp>
#include <engine/render/skybox.hpp>
#include <engine/scene/draw_list_gen.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
Rect2D cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}, true}; }

void addNodes(DrawListFactory::GroupMap& map, DrawGroup const& group, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active) {
			if (auto props = node->props(); !props.empty()) {
				Rect2D const rect = cast(graphics::utils::scissor(node->m_scissor));
				map[group].push_back({node->model(), rect, props});
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, group, *node); }
	}
}

void addNodes(DrawListFactory::GroupMap2& map, DrawGroup const& group, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active) {
			if (auto mesh = node->mesh(); !mesh.empty()) {
				Rect2D const rect = cast(graphics::utils::scissor(node->m_scissor));
				map[group].push_back({node->model(), rect, mesh});
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, group, *node); }
	}
}
} // namespace

void DrawListGen3D::operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	for (auto [_, c] : registry.view<DrawGroup, SceneNode, Prop>(exclude)) {
		auto& [group, node, prop] = c;
		if (prop.primitive) { map[group].push_back({node.model(registry), {}, prop}); }
	}
	for (auto [_, c] : registry.view<DrawGroup, SceneNode, PropProvider>(exclude)) {
		auto& [group, node, provider] = c;
		auto props = provider.props();
		if (!props.empty()) { map[group].push_back({node.model(registry), {}, props}); }
	}
	for (auto [_, c] : registry.view<DrawGroup, Skybox>(exclude)) {
		auto& [group, skybox] = c;
		map[group].push_back({glm::mat4(1.0f), {}, skybox.prop()});
	}
	for (auto [_, c] : registry.view<DrawGroup, physics::Trigger::Debug>(exclude)) {
		auto& [group, physics] = c;
		if (auto drawables = physics.drawables(registry); !drawables.empty()) { std::move(drawables.begin(), drawables.end(), std::back_inserter(map[group])); }
	}
}

void DrawListGen::operator()(DrawListFactory::GroupMap2& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	/*for (auto [_, c] : registry.view<DrawGroup, SceneNode, Prop>(exclude)) {
		auto& [group, node, prop] = c;
		if (prop.primitive) { map[group].push_back({node.model(registry), {}, prop}); }
	}
	for (auto [_, c] : registry.view<DrawGroup, SceneNode, PropProvider>(exclude)) {
		auto& [group, node, provider] = c;
		auto props = provider.props();
		if (!props.empty()) { map[group].push_back({node.model(registry), {}, props}); }
	}*/
	for (auto [_, c] : registry.view<DrawGroup, SceneNode, DynamicMesh>(exclude)) {
		auto& [group, node, mesh] = c;
		map[group].push_back({node.model(registry), {}, mesh.mesh()});
	}
	for (auto [e, c] : registry.view<DrawGroup, MeshProvider>(exclude)) {
		auto& [group, provider] = c;
		auto const node = registry.find<SceneNode>(e);
		glm::mat4 const model = node ? node->model(registry) : glm::mat4(1.0f);
		map[group].push_back({model, {}, provider.mesh()});
	}
	for (auto [_, c] : registry.view<DrawGroup, physics::Trigger::Debug>(exclude)) {
		auto& [group, physics] = c;
		if (auto drawables = physics.drawables2(registry); !drawables.empty()) {
			std::move(drawables.begin(), drawables.end(), std::back_inserter(map[group]));
		}
	}
	for (auto& [_, c] : registry.view<DrawGroup, gui::ViewStack>()) {
		auto& [gr, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, gr, *view); }
	}
}

void DrawListGenUI::operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const {
	for (auto& [_, c] : registry.view<DrawGroup, gui::ViewStack>()) {
		auto& [gr, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, gr, *view); }
	}
}
} // namespace le
