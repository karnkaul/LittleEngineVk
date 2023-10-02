#pragma once
#include <le/error.hpp>
#include <le/node/node.hpp>
#include <le/scene/component_map.hpp>
#include <unordered_set>

namespace le {
class Entity final : public ComponentMap {
  public:
	explicit Entity(NotNull<Scene*> scene, NotNull<Node const*> node);

	[[nodiscard]] auto id() const -> Id<Entity> { return m_id; }
	[[nodiscard]] auto node_id() const -> Id<Node> { return m_node_id; }

	[[nodiscard]] auto get_node() const noexcept(false) -> Node const&;
	[[nodiscard]] auto get_node() noexcept(false) -> Node&;

	[[nodiscard]] auto get_transform() const noexcept(false) -> Transform const& { return get_node().transform; }
	[[nodiscard]] auto get_transform() noexcept(false) -> Transform& { return get_node().transform; }

	[[nodiscard]] auto get_scene() const -> Scene& { return *m_scene; }

	template <std::derived_from<Component> ComponentT>
	auto attach(std::unique_ptr<ComponentT> component) -> ComponentT& {
		if (!component) { throw Error{"attempt to attach a null component"}; }
		auto* ret = component.get();
		insert_or_assign(std::move(component));
		return *ret;
	}

	template <std::derived_from<Component> ComponentT, typename... Args>
		requires(std::constructible_from<ComponentT, Entity&, Args...>)
	auto attach(Args&&... args) -> ComponentT& {
		auto component = std::make_unique<ComponentT>(*this, std::forward<Args>(args)...);
		auto* ret = component.get();
		insert_or_assign(std::move(component));
		return *ret;
	}

	[[nodiscard]] auto global_position() const -> glm::vec3;

	[[nodiscard]] auto is_active() const { return m_active; }
	auto set_active(bool active) { m_active = active; }

	[[nodiscard]] auto is_destroyed() const { return m_destroyed; }
	auto set_destroyed() { m_destroyed = true; }

	auto tick(Duration dt) -> void;

	auto fill(std::vector<NotNull<RenderComponent const*>>& out) const -> void;

  private:
	NotNull<Scene*> m_scene;
	Id<Entity> m_id;
	Id<Node> m_node_id;

	std::vector<Ptr<Component>> m_cache{};
	Id<Component>::id_type m_next_component_id{};
	bool m_active{true};
	bool m_destroyed{};
};

using EntityId = Id<Entity>;
} // namespace le
