#pragma once
#include <le/core/time.hpp>
#include <le/graphics/render_object.hpp>
#include <le/scene/ui/rect_transform.hpp>
#include <memory>

namespace le::ui {
using graphics::Material;
using graphics::Primitive;
using graphics::RenderObject;

class View {
  public:
	View() = default;
	View(View const&) = delete;
	View(View&&) = delete;
	auto operator=(View const&) -> View& = delete;
	auto operator=(View&&) -> View& = delete;

	virtual ~View() = default;

	virtual auto setup() -> void {}
	virtual auto tick(Duration dt) -> void;
	virtual auto render_tree(std::vector<RenderObject>& out) const -> void;

	auto push_sub_view(std::unique_ptr<View> sub_view) -> void;

	[[nodiscard]] auto is_destroyed() const -> bool { return m_destroyed; }
	auto set_destroyed() -> void { m_destroyed = true; }

	[[nodiscard]] auto get_parent() const -> Ptr<View> { return m_parent; }

	[[nodiscard]] auto get_parent_mat() const -> glm::mat4 { return m_parent_mat; }

	RectTransform transform{};

  private:
	std::vector<std::unique_ptr<View>> m_sub_views{};
	glm::mat4 m_parent_mat{1.0f};
	std::vector<Ptr<View>> m_cache{};
	Ptr<View> m_parent{};
	bool m_destroyed{};
};
} // namespace le::ui
