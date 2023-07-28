#pragma once
#include <spaced/error.hpp>
#include <spaced/node/node.hpp>
#include <spaced/scene/render_component.hpp>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace spaced {
class Entity final {
  public:
	Entity(Entity&&) = default;
	auto operator=(Entity&&) -> Entity& = default;

	Entity(Entity const&) = delete;
	auto operator=(Entity const&) -> Entity& = delete;

	explicit Entity(NotNull<Scene*> scene, NotNull<Node const*> node);
	~Entity();

	[[nodiscard]] auto id() const -> Id<Entity> { return m_id; }

	[[nodiscard]] auto get_node() const noexcept(false) -> Node const&;
	[[nodiscard]] auto get_node() noexcept(false) -> Node&;

	[[nodiscard]] auto get_transform() const noexcept(false) -> Transform const& { return get_node().transform; }
	[[nodiscard]] auto get_transform() noexcept(false) -> Transform& { return get_node().transform; }

	[[nodiscard]] auto get_scene() const -> Scene& { return *m_scene; }

	template <std::derived_from<Component> ComponentT>
	auto attach(std::unique_ptr<ComponentT> component) -> ComponentT& {
		if (!component) { throw Error{"attempt to attach a null component"}; }
		component->initialize(m_scene, this, m_next_component_id++);
		auto* ret = component.get();
		auto comp = Comp{.component = std::move(component)};
		if constexpr (std::derived_from<ComponentT, RenderComponent>) { comp.render_component = ret; }
		m_components.insert_or_assign(typeid(ComponentT), std::move(comp));
		return *ret;
	}

	template <std::derived_from<Component> ComponentT>
	auto detach() -> void {
		m_components.erase(typeid(ComponentT));
	}

	template <std::derived_from<Component> ComponentT>
	[[nodiscard]] auto has_component() const -> bool {
		return find_component<ComponentT>() != nullptr;
	}

	template <std::derived_from<Component> ComponentT>
	[[nodiscard]] auto find_component() const -> Ptr<ComponentT> {
		if (auto it = m_components.find(typeid(ComponentT)); it != m_components.end()) { return static_cast<ComponentT*>(it->second.component.get()); }
		return nullptr;
	}

	template <std::derived_from<Component> ComponentT>
	[[nodiscard]] auto get_component() const -> Component& {
		auto* ret = find_component<ComponentT>();
		if (!ret) { throw Error{"requested component not attached"}; }
		return *ret;
	}

	auto clear_components() -> void { m_components.clear(); }
	[[nodiscard]] auto component_count() const -> std::size_t { return m_components.size(); }

	[[nodiscard]] auto is_active() const { return m_active; }
	auto set_active(bool active) { m_active = active; }

	[[nodiscard]] auto is_destroyed() const { return m_destroyed; }
	auto set_destroyed() { m_destroyed = true; }

	auto tick(Duration dt) -> void;

  private:
	struct Comp {
		std::unique_ptr<Component> component{};
		Ptr<RenderComponent> render_component{};
	};

	auto fill(std::vector<NotNull<RenderComponent const*>>& out) const -> void;

	std::unordered_map<std::type_index, Comp> m_components{};
	std::vector<Ptr<Component>> m_cache{};
	NotNull<Scene*> m_scene;
	Id<Entity> m_id;
	Id<Node> m_node_id;
	Id<Component>::id_type m_next_component_id{};
	bool m_active{true};
	bool m_destroyed{};

	friend class Scene;
};
} // namespace spaced
