#include <spaced/scene/ui/view.hpp>
#include <algorithm>

namespace spaced::ui {
// NOLINTNEXTLINE
auto View::tick(Duration dt) -> void {
	m_cache.clear();
	m_cache.reserve(m_sub_views.size());
	for (auto const& sub_view : m_sub_views) { m_cache.push_back(sub_view.get()); }

	for (auto* sub_view : m_cache) {
		auto offset_transform = transform;
		offset_transform.position += sub_view->transform.anchor * transform.extent;
		sub_view->m_parent_mat = get_parent_mat() * offset_transform.matrix();
		sub_view->tick(dt);
	}
	std::erase_if(m_sub_views, [](auto const& sub_view) { return sub_view->is_destroyed(); });
}

// NOLINTNEXTLINE
auto View::render_tree(std::vector<RenderObject>& out) const -> void {
	for (auto const* view : m_cache) { view->render_tree(out); }
}

auto View::push_sub_view(std::unique_ptr<View> sub_view) -> void {
	if (!sub_view) { return; }
	sub_view->m_parent = this;
	sub_view->setup();
	m_sub_views.push_back(std::move(sub_view));
}
} // namespace spaced::ui
