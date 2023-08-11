#pragma once
#include <le/scene/ui/element.hpp>
#include <memory>

namespace le::ui {
class Quad;

class View {
  public:
	View() = default;
	View(View const&) = delete;
	View(View&&) = delete;
	auto operator=(View const&) -> View& = delete;
	auto operator=(View&&) -> View& = delete;

	virtual ~View() = default;

	virtual auto setup() -> void;
	virtual auto tick(Duration dt) -> void;
	virtual auto render_tree(std::vector<RenderObject>& out) const -> void;

	auto push_element(std::unique_ptr<Element> element) -> void;
	auto push_sub_view(std::unique_ptr<View> sub_view) -> void;

	[[nodiscard]] auto get_background() const -> std::optional<graphics::Rgba>;
	auto set_background(graphics::Rgba tint = graphics::white_v) -> void;
	auto reset_background() -> void;

	[[nodiscard]] auto is_destroyed() const -> bool { return m_destroyed; }
	auto set_destroyed() -> void { m_destroyed = true; }

	[[nodiscard]] auto get_parent() const -> Ptr<View> { return m_parent; }
	[[nodiscard]] auto get_parent_matrix() const -> glm::mat4 { return m_parent_mat; }

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
