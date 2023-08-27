#include <le/engine.hpp>
#include <le/error.hpp>
#include <le/scene/scene.hpp>
#include <algorithm>
#include <format>
#include <utility>

namespace le {
Scene::~Scene() {
	// subcomponent destructors might call get_entity() / get_scene(), need to invoke them before this destructor exits
	m_entity_map.clear();
}

auto Scene::spawn(std::string name, Ptr<Entity> parent) -> Entity& {
	auto const parent_id = parent != nullptr ? std::optional<Id<Node>>{parent->node_id()} : std::nullopt;
	auto& node = m_node_tree.add(NodeCreateInfo{.name = std::move(name), .parent = parent_id});
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
	if (ret == nullptr) { throw Error{std::format("invalid Id<Entity>: {}", id.value())}; }
	return *ret;
}

// NOLINTNEXTLINE
auto Scene::get_entity(Id<Entity> id) -> Entity& { return const_cast<Entity&>(std::as_const(*this).get_entity(id)); }

auto Scene::reparent(Entity& entity, Entity& parent) -> void { m_node_tree.reparent(entity.get_node(), parent.node_id()); }

auto Scene::unparent(Entity& entity) -> void { m_node_tree.reparent(entity.get_node(), {}); }

auto Scene::clear_entities() -> void {
	m_entity_map.clear();
	m_node_tree.clear();
	m_active.entities.clear();
	m_active.render_components.clear();
	m_destroyed.clear();
}

auto Scene::setup() -> void {
	m_ui_root.transform.extent = Engine::self().framebuffer_extent();
	collision.setup();
}

auto Scene::tick(Duration dt) -> void {
	// clear cache
	m_active.entities.clear();
	m_destroyed.clear();

	// cache all non-destroyed and active entities
	for (auto& [id, entity] : m_entity_map) {
		if (!entity.is_active() || entity.is_destroyed()) { continue; }
		m_active.entities.push_back(&entity);
	}

	// sort by order of spawning
	std::ranges::sort(m_active.entities, [](Ptr<Entity const> a, Ptr<Entity const> b) { return a->id() < b->id(); });
	// tick
	for (auto* entity : m_active.entities) { entity->tick(dt); }

	// cache destroyed entity IDs
	for (auto& [id, entity] : m_entity_map) {
		if (entity.is_destroyed()) {
			m_destroyed.emplace_back(id);
			continue;
		}
	}

	// remove destroyed nodes, children, and their entities
	auto const destroy_entity = [this](Node const& node) { m_entity_map.erase(*node.entity_id); };
	for (auto const destroyed : m_destroyed) {
		auto const& entity = m_entity_map.at(destroyed);
		m_node_tree.remove(entity.node_id(), destroy_entity);
	}

	m_ui_root.transform.extent = Engine::self().framebuffer_extent();
	m_ui_root.tick(dt);

	collision.tick(*this, dt);
}

auto Scene::render_entities(std::vector<graphics::RenderObject>& out) const -> void {
	// clear cache
	m_active.render_components.clear();

	for (auto const& [id, entity] : m_entity_map) {
		if (!entity.is_active()) { continue; }
		entity.fill(m_active.render_components);
	}

	// sort render components in order of layers (since render_entities() is const)
	std::ranges::sort(m_active.render_components, [](Ptr<RenderComponent const> a, Ptr<RenderComponent const> b) { return a->render_layer < b->render_layer; });

	for (auto const& render_component : m_active.render_components) { render_component->render_to(out); }

	// render collision AABBs
	collision.render_to(out);
}
} // namespace le
