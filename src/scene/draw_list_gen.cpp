#include <dens/registry.hpp>
#include <engine/ecs/components/trigger.hpp>
#include <engine/gui/tree.hpp>
#include <engine/gui/view.hpp>
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
} // namespace

void DrawListGen3D::operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	for (auto [_, c] : registry.view<DrawGroup, SceneNode, Prop>(exclude)) {
		auto& [group, node, prop] = c;
		if (prop.mesh) { map[group].push_back({node.model(registry), {}, prop}); }
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

void DrawListGenUI::operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const {
	for (auto& [_, c] : registry.view<DrawGroup, gui::ViewStack>()) {
		auto& [gr, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, gr, *view); }
	}
}
} // namespace le
