#pragma once
#include <spaced/core/time.hpp>
#include <spaced/graphics/render_object.hpp>
#include <spaced/scene/ui/rect_transform.hpp>
#include <memory>

namespace spaced::ui {
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
	virtual auto render_tree(Rect2D<> const& parent, std::vector<graphics::RenderObject>& out) const -> void;

	auto push_sub_view(std::unique_ptr<View> sub_view) -> void;

	[[nodiscard]] auto is_destroyed() const -> bool { return m_destroyed; }
	auto set_destroyed() -> void { m_destroyed = true; }

	[[nodiscard]] auto get_parent() const -> Ptr<View> { return m_parent; }

	RectTransform transform{};

  private:
	std::vector<std::unique_ptr<View>> m_sub_views{};
	std::vector<Ptr<View>> m_cache{};
	Ptr<View> m_parent{};
	bool m_destroyed{};
};
} // namespace spaced::ui
