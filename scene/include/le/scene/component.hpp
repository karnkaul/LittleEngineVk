#pragma once
#include <le/core/id.hpp>
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

	Component(Entity& entity);
	virtual ~Component() = default;

	[[nodiscard]] auto get_entity() const noexcept(false) -> Entity&;
	[[nodiscard]] auto get_scene() const noexcept(false) -> Scene&;

	[[nodiscard]] auto id() const -> Id<Component> { return m_self_id; }

	virtual auto tick(Duration dt) -> void = 0;

  private:
	Id<Component> m_self_id;
	Ptr<Scene> m_scene{};
	std::optional<Id<Entity>> m_entity_id{};

	inline static Id<Component>::id_type s_prev_id{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};
} // namespace le
