#include <le/error.hpp>
#include <le/scene/component.hpp>
#include <le/scene/scene.hpp>
#include <format>

namespace le {
auto Component::get_entity() const -> Entity& {
	if (m_scene == nullptr || !m_entity_id) { throw Error{"get_entity() called on an orphan component"}; }
	return m_scene->get_entity(*m_entity_id);
}

auto Component::get_scene() const -> Scene& {
	if (m_scene == nullptr) { throw Error{"get_scene() called on an orphan component"}; }
	return *m_scene;
}

auto Component::initialize(NotNull<Scene*> scene, NotNull<Entity*> entity, Id<Component> self_id) -> void {
	m_scene = scene;
	m_entity_id = entity->id();
	m_self_id = self_id;
	setup();
}
} // namespace le
