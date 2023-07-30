#include <spaced/scene/ui/view.hpp>
#include <algorithm>

namespace spaced::ui {
// NOLINTNEXTLINE
auto View::tick(Duration dt) -> void {
	m_cache.clear();
	m_cache.reserve(m_sub_views.size());
	for (auto const& sub_view : m_sub_views) { m_cache.push_back(sub_view.get()); }

	for (auto* sub_view : m_cache) { sub_view->tick(dt); }
	std::erase_if(m_sub_views, [](auto const& sub_view) { return sub_view->is_destroyed(); });
}

// NOLINTNEXTLINE
auto View::render_tree(Rect2D<> const& parent, std::vector<graphics::RenderObject>& out) const -> void {
	auto frame = transform.merge_frame(parent);
	for (auto const* view : m_cache) { view->render_tree(frame, out); }
}

auto View::push_sub_view(std::unique_ptr<View> sub_view) -> void {
	if (!sub_view) { return; }
	sub_view->m_parent = this;
	sub_view->setup();
	m_sub_views.push_back(std::move(sub_view));
}
} // namespace spaced::ui
