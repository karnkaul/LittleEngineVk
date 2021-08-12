#include <dumb_ecf/registry.hpp>
#include <engine/gui/tree.hpp>
#include <engine/gui/view.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <engine/scene/scene_node.hpp>
#include <engine/scene/skybox.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
Rect2D cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}, true}; }
} // namespace

void DrawListGen3D::operator()(DrawListFactory::LayerMap& map, decf::registry const& registry) const {
	for (auto& [_, c] : registry.view<DrawLayer, SceneNode, PropList>()) {
		auto& [layer, node, props] = c;
		if (!props.empty() && layer.pipeline) { map[layer].push_back({node.model(), {}, props}); }
	}
	for (auto& [_, c] : registry.view<DrawLayer, Skybox>()) {
		auto& [layer, skybox] = c;
		if (layer.pipeline) { map[layer].push_back({glm::mat4(1.0f), {}, skybox.prop()}); }
	}
}

void DrawListGenUI::operator()(DrawListFactory::LayerMap& map, decf::registry const& registry) const {
	for (auto& [_, c] : registry.view<DrawLayer, gui::ViewStack>()) {
		auto& [gr, stack] = c;
		for (auto const& view : stack.views()) { DrawListFactory::add(map, gr, *view); }
	}
}

void DrawListFactory::add(LayerMap& map, DrawLayer const& layer, gui::TreeRoot const& root) {
	for (auto& node : root.nodes()) {
		if (node->m_active) {
			if (auto props = node->props(); !props.empty()) {
				Rect2D const rect = cast(graphics::utils::scissor(node->m_scissor));
				map[layer].push_back({node->model(), rect, props});
			}
		}
	}
	for (auto& node : root.nodes()) {
		if (node->m_active) { add(map, layer, *node); }
	}
}

void DrawListFactory::attach(decf::registry& registry, decf::entity entity, DrawLayer layer, Span<Prop const> props) {
	registry.attach<PropList>(entity) = props;
	registry.attach<DrawLayer>(entity, layer);
}
} // namespace le
