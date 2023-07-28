#include <spaced/error.hpp>
#include <spaced/scene/scene.hpp>
#include <algorithm>
#include <format>
#include <utility>

namespace spaced {
Scene::~Scene() {
	// subcomponent destructors might call get_entity() / get_scene(), need to invoke them before this destructor exits
	m_entity_map.clear();
}

auto Scene::spawn(std::string name) -> Entity& {
	auto& node = m_node_tree.add(NodeCreateInfo{.name = std::move(name)});
	node.entity_id = m_next_id++;
	auto [it, _] = m_entity_map.insert_or_assign(*node.entity_id, Entity{this, &node});
	return it->second;
}

auto Scene::find_entity(Id<Entity> id) const -> Ptr<Entity const> {
	if (auto it = m_entity_map.find(id); it != m_entity_map.end()) { return &it->second; }
	return {};
}

// NOLINTNEXTLINE
auto Scene::find_entity(Id<Entity> id) -> Ptr<Entity> { return const_cast<Entity*>(std::as_const(*this).find_entity(id)); }

auto Scene::get_entity(Id<Entity> id) const -> Entity const& {
	auto const* ret = find_entity(id);
	if (ret == nullptr) { throw Error{std::format("nonexistent entity: {}", id.value())}; }
	return *ret;
}

// NOLINTNEXTLINE
auto Scene::get_entity(Id<Entity> id) -> Entity& { return const_cast<Entity&>(std::as_const(*this).get_entity(id)); }

auto Scene::clear_entities() -> void {
	m_entity_map.clear();
	m_node_tree.clear();
	m_active.entities.clear();
	m_active.render_components.clear();
	m_destroyed.clear();
}

auto Scene::tick(Duration dt) -> void {
	// clear cache
	m_active.entities.clear();
	m_active.render_components.clear();
	m_destroyed.clear();

	// cache all non-destroyed and active entities
	for (auto& [id, entity] : m_entity_map) {
		if (!entity.is_active() || entity.is_destroyed()) { continue; }
		m_active.entities.push_back(&entity);
	}

	// sort by order of spawning
	std::ranges::sort(m_active.entities, [](Ptr<Entity> a, Ptr<Entity> b) { return a->id() < b->id(); });
	// tick
	for (auto* entity : m_active.entities) { entity->tick(dt); }

	// cache destroyed entity IDs and remove their nodes
	for (auto& [id, entity] : m_entity_map) {
		if (entity.is_destroyed()) {
			m_destroyed.push_back(id);
			m_node_tree.remove(entity.m_node_id);
			continue;
		}
		// this is done so late in order to discard all destroyed entities
		entity.fill(m_active.render_components);
	}
	// remove destroyed entities
	for (auto const destroyed : m_destroyed) { m_entity_map.erase(destroyed); }

	// sort render components in order of layers (since render_to() is const)
	std::ranges::sort(m_active.render_components, [](Ptr<RenderComponent const> a, Ptr<RenderComponent const> b) { return a->layer < b->layer; });
}

auto Scene::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	for (auto const& render_component : m_active.render_components) { render_component->render_to(out); }
}
} // namespace spaced
