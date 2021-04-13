#include <core/utils/std_hash.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/gui/node.hpp>
#include <engine/scene/scene_drawer.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
std::size_t DrawGroup::Hasher::operator()(DrawGroup const& gr) const noexcept {
	return std::hash<graphics::Pipeline const*>{}(gr.pipeline) ^ (std::hash<s64>{}(gr.order) << 1);
}

void SceneDrawer::Populator::operator()(ItemMap& map, decf::registry_t const& registry) const {
	for (auto& [_, d] : registry.view<DrawGroup, SceneNode, PrimList>()) {
		auto& [gr, node, pl] = d;
		if (!pl.empty() && gr.pipeline) {
			map[gr].push_back({node.model(), std::nullopt, pl});
		}
	}
	for (auto& [_, d] : registry.view<DrawGroup, gui::Root>()) {
		auto& [gr, root] = d;
		add(map, gr, root);
	}
}

void SceneDrawer::add(ItemMap& map, DrawGroup const& group, gui::Root const& root) {
	for (auto& node : root.m_nodes) {
		if (auto prims = node->primitives(); !prims.empty()) {
			map[group].push_back({node->model(), graphics::utils::scissor(node->m_scissor), prims});
		}
	}
	for (auto& node : root.m_nodes) {
		add(map, group, *node);
	}
}

void SceneDrawer::sort(Span<Group> items) noexcept {
	std::sort(items.begin(), items.end(), [](Group const& a, Group const& b) { return a.group.order < b.group.order; });
}

void SceneDrawer::attach(decf::registry_t& reg, decf::entity_t entity, DrawGroup const& group, View<Primitive> primitives) {
	reg.attach<PrimList>(entity) = {primitives.begin(), primitives.end()};
	reg.attach<DrawGroup>(entity, group);
}
} // namespace le
