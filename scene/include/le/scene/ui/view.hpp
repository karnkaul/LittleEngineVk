#pragma once
#include <le/scene/ui/element.hpp>
#include <memory>

namespace le::ui {
class Quad;

class View : public Polymorphic {
  public:
	explicit View(Ptr<View> parent_view = {});

	virtual auto tick(Duration dt) -> void;
	virtual auto render_tree(std::vector<RenderObject>& out) const -> void;

	template <std::derived_from<Element> Type, typename... Args>
		requires(std::constructible_from<Type, View*, Args...>)
	auto push_element(Args&&... args) -> Type& {
		auto element = std::make_unique<Type>(this, std::forward<Args>(args)...);
		auto& ret = *element;
		m_elements.push_back(std::move(element));
		return ret;
	}

	template <std::derived_from<View> Type, typename... Args>
		requires(std::constructible_from<Type, View*, Args...>)
	auto push_sub_view(Args&&... args) -> Type& {
		auto view = std::make_unique<Type>(this, std::forward<Args>(args)...);
		auto& ret = *view;
		m_sub_views.push_back(std::move(view));
		return ret;
	}

	[[nodiscard]] auto get_background() const -> std::optional<graphics::Rgba>;
	auto set_background(graphics::Rgba tint = graphics::white_v) -> void;
	auto reset_background() -> void;

	[[nodiscard]] auto is_destroyed() const -> bool { return m_destroyed; }
	auto set_destroyed() -> void { m_destroyed = true; }

	[[nodiscard]] auto get_parent() const -> Ptr<View> { return m_parent; }
	[[nodiscard]] auto get_parent_matrix() const -> glm::mat4 { return m_parent_mat; }
	[[nodiscard]] auto global_position() const -> glm::vec3;

	RectTransform transform{};

  private:
	std::vector<std::unique_ptr<Element>> m_elements{};
	std::vector<std::unique_ptr<View>> m_sub_views{};
	glm::mat4 m_parent_mat{1.0f};
	std::vector<Ptr<View>> m_view_cache{};
	std::vector<Ptr<Element>> m_element_cache{};
	Ptr<View> m_parent{};
	Ptr<Quad> m_background{};
	bool m_destroyed{};
};
} // namespace le::ui
