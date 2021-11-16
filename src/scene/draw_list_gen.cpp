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

void addNodes(DrawListFactory::LayerMap& map, DrawLayer const& layer, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active) {
			if (auto props = node->props(); !props.empty()) {
				Rect2D const rect = cast(graphics::utils::scissor(node->m_scissor));
				map[layer].push_back({node->model(), rect, props});
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { addNodes(map, layer, *node); }
	}
}
} // namespace

void DrawListGen3D::operator()(DrawListFactory::LayerMap& map, dens::registry const& registry) const {
	static constexpr auto exclude = dens::exclude<NoDraw>();
	for (auto [_, c] : registry.view<DrawLayer, SceneNode, Prop>(exclude)) {
		auto& [layer, node, prop] = c;
		if (prop.mesh && layer.pipeline) { map[layer].push_back({node.model(registry), {}, prop}); }
	}
	for (auto [_, c] : registry.view<DrawLayer, SceneNode, PropProvider>(exclude)) {
		auto& [layer, node, provider] = c;
		auto props = provider.props();
		if (!props.empty() && layer.pipeline) { map[layer].push_back({node.model(registry), {}, props}); }
	}
	for (auto [_, c] : registry.view<DrawLayer, Skybox>(exclude)) {
		auto& [layer, skybox] = c;
		if (layer.pipeline) { map[layer].push_back({glm::mat4(1.0f), {}, skybox.prop()}); }
	}
	for (auto [_, c] : registry.view<DrawLayer, physics::Trigger::Debug>(exclude)) {
		auto& [layer, physics] = c;
		if (layer.pipeline) {
			if (auto drawables = physics.drawables(registry); !drawables.empty()) {
				std::move(drawables.begin(), drawables.end(), std::back_inserter(map[layer]));
			}
		}
	}
}

void DrawListGenUI::operator()(DrawListFactory::LayerMap& map, dens::registry const& registry) const {
	for (auto& [_, c] : registry.view<DrawLayer, gui::ViewStack>()) {
		auto& [gr, stack] = c;
		for (auto const& view : stack.views()) { addNodes(map, gr, *view); }
	}
}
} // namespace le
