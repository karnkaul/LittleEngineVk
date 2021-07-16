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
	for (auto& [_, c] : registry.view<DrawLayer, SceneNode, PrimitiveList>()) {
		auto& [layer, node, prims] = c;
		if (!prims.empty() && layer.pipeline) { map[layer].push_back({node.model(), {}, prims}); }
	}
	for (auto& [_, c] : registry.view<DrawLayer, Skybox>()) {
		auto& [layer, skybox] = c;
		if (layer.pipeline) { map[layer].push_back({glm::mat4(1.0f), {}, skybox.primitive()}); }
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
		if (auto prims = node->primitives(); !prims.empty()) {
			Rect2D const rect = cast(graphics::utils::scissor(node->m_scissor));
			map[layer].push_back({node->model(), rect, prims});
		}
	}
	for (auto& node : root.nodes()) { add(map, layer, *node); }
}

void DrawListFactory::attach(decf::registry& registry, decf::entity entity, DrawLayer layer, Span<Primitive const> primitives) {
	registry.attach<PrimitiveList>(entity) = primitives;
	registry.attach<DrawLayer>(entity, layer);
}
} // namespace le
