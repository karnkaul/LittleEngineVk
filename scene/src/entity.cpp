#include <le/scene/entity.hpp>
#include <le/scene/scene.hpp>
#include <algorithm>

namespace le {
Entity::Entity(NotNull<Scene*> scene, NotNull<Node const*> node) : m_scene(scene), m_id(*node->entity_id), m_node_id(node->id()) {}

auto Entity::get_node() const -> Node const& { return m_scene->get_node_tree().get(m_node_id); }
auto Entity::get_node() -> Node& { return m_scene->make_node_locator().get(m_node_id); }

auto Entity::global_position() const -> glm::vec3 { return m_scene->get_node_tree().global_position(get_node()); }

auto Entity::tick(Duration dt) -> void {
	m_cache.clear();
	m_cache.reserve(m_components.size());
	for (auto const& [_, comp] : m_components) { m_cache.push_back(comp.component.get()); }
	// sort by order of attachment before ticking
	std::ranges::sort(m_cache, [](Ptr<Component> a, Ptr<Component> b) { return a->m_self_id < b->m_self_id; });
	for (auto* component : m_cache) { component->tick(dt); }
}

auto Entity::fill(std::vector<NotNull<RenderComponent const*>>& out) const -> void {
	for (auto const& [_, comp] : m_components) {
		if (comp.render_component != nullptr) { out.emplace_back(comp.render_component); }
	}
}
} // namespace le
