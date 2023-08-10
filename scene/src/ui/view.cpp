#include <le/scene/ui/view.hpp>
#include <algorithm>

namespace le::ui {
auto View::tick(Duration dt) -> void { // NOLINT
	auto do_tick = [this, dt](auto& cache, auto& source) {
		cache.clear();
		cache.reserve(source.size());
		for (auto const& t : source) { cache.push_back(t.get()); }

		for (auto* t : cache) {
			auto offset_transform = transform;
			offset_transform.position += t->transform.anchor * transform.extent;
			t->m_parent_mat = get_parent_matrix() * offset_transform.matrix();
			t->tick(dt);
		}

		std::erase_if(source, [](auto const& t) { return t->is_destroyed(); });
	};

	do_tick(m_element_cache, m_elements);
	do_tick(m_view_cache, m_sub_views);
}

auto View::render_tree(std::vector<RenderObject>& out) const -> void { // NOLINT
	for (auto const& element : m_elements) { element->render(out); }
	for (auto const& view : m_sub_views) { view->render_tree(out); }
}

auto View::push_element(std::unique_ptr<Element> element) -> void {
	if (!element) { return; }
	element->m_parent_view = this;
	element->setup();
	m_elements.push_back(std::move(element));
}

auto View::push_sub_view(std::unique_ptr<View> sub_view) -> void {
	if (!sub_view) { return; }
	sub_view->m_parent = this;
	sub_view->setup();
	m_sub_views.push_back(std::move(sub_view));
}
} // namespace le::ui
