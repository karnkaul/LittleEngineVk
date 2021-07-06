#include <dumb_ecf/registry.hpp>
#include <engine/gui/tree.hpp>
#include <engine/gui/view.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
Rect2D cast(vk::Rect2D r) noexcept { return {{r.extent.width, r.extent.height}, {r.offset.x, r.offset.y}, true}; }
} // namespace

void DrawListGen3D::operator()(DrawListFactory::LayerMap& map, decf::registry_t const& registry) const {
	for (auto& [_, d] : registry.view<DrawLayer, SceneNode, std::vector<Primitive>>()) {
		auto& [layer, node, prims] = d;
		if (!prims.empty() && layer.pipeline) { map[layer].push_back({node.model(), {}, prims}); }
	}
}

void DrawListGenUI::operator()(DrawListFactory::LayerMap& map, decf::registry_t const& registry) const {
	for (auto& [_, d] : registry.view<DrawLayer, gui::ViewStack>()) {
		auto& [gr, stack] = d;
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

void DrawListFactory::attach(decf::registry_t& registry, decf::entity_t entity, DrawLayer layer, Span<Primitive const> primitives) {
	registry.attach<std::vector<Primitive>>(entity) = {primitives.begin(), primitives.end()};
	registry.attach<DrawLayer>(entity, layer);
}
} // namespace le
