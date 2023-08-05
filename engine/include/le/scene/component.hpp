#pragma once
#include <le/core/id.hpp>
#include <le/core/not_null.hpp>
#include <le/core/ptr.hpp>
#include <le/core/time.hpp>
#include <optional>

namespace le {
class Entity;
class Scene;

using EntityId = Id<Entity>;

class Component {
  public:
	Component(Component const&) = delete;
	Component(Component&&) = delete;
	auto operator=(Component const&) -> Component& = delete;
	auto operator=(Component&&) -> Component& = delete;

	Component() = default;
	virtual ~Component() = default;

	[[nodiscard]] auto get_entity() const noexcept(false) -> Entity&;
	[[nodiscard]] auto get_scene() const noexcept(false) -> Scene&;

	virtual auto setup() -> void {}
	virtual auto tick(Duration dt) -> void = 0;

  private:
	auto initialize(NotNull<Scene*> scene, NotNull<Entity*> entity, Id<Component> self_id) -> void;

	Ptr<Scene> m_scene{};
	std::optional<Id<Entity>> m_entity_id{};
	std::optional<Id<Component>> m_self_id{};

	friend class Entity;
};
} // namespace le
