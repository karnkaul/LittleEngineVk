#pragma once
#include <le/scene/render_component.hpp>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace le {
class ComponentMap {
  public:
	ComponentMap(ComponentMap const&) = delete;
	auto operator=(ComponentMap const&) -> ComponentMap& = delete;

	ComponentMap(ComponentMap&&) = default;
	auto operator=(ComponentMap&&) -> ComponentMap& = default;

	explicit ComponentMap() = default;

	virtual ~ComponentMap() { m_components.clear(); }

	template <std::derived_from<Component> ComponentT>
	auto insert_or_assign(std::unique_ptr<ComponentT> component) -> void {
		if (!component) { return; }
		auto comp = Comp{};
		if constexpr (std::derived_from<ComponentT, RenderComponent>) { comp.render_component = component.get(); }
		comp.component = std::move(component);
		m_components.insert_or_assign(typeid(ComponentT), std::move(comp));
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
		for (auto const& [_, comp] : m_components) {
			if (auto* ret = dynamic_cast<ComponentT*>(comp.component.get())) { return ret; }
		}
		return nullptr;
	}

	template <std::derived_from<Component> ComponentT>
	[[nodiscard]] auto get_component() const -> ComponentT& {
		auto* ret = find_component<ComponentT>();
		if (!ret) { throw Error{"requested component not attached"}; }
		return *ret;
	}

	template <std::derived_from<Component> ComponentT>
	[[nodiscard]] auto get_components() const -> std::vector<NotNull<ComponentT*>> {
		auto ret = std::vector<NotNull<ComponentT*>>{};
		for (auto const& [_, comp] : m_components) {
			if (auto* component = dynamic_cast<ComponentT*>(comp.component.get())) { ret.emplace_back(component); }
		}
		return ret;
	}

	auto clear_components() -> void { m_components.clear(); }
	[[nodiscard]] auto component_count() const -> std::size_t { return m_components.size(); }

  protected:
	struct Comp {
		std::unique_ptr<Component> component{};
		Ptr<RenderComponent> render_component{};
	};

	std::unordered_map<std::type_index, Comp> m_components{};
};
} // namespace le
