#pragma once
#include <engine/gui/widget.hpp>

namespace le::gui {
class Flexbox : public Widget {
  public:
	enum class Axis { eHorz, eVert };

	Flexbox(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, Axis axis = {}) noexcept : gui::Widget(root, font), m_axis(axis) { m_rect.size = {}; }

	template <typename T = Widget, typename... Args>
		requires(std::is_base_of_v<Widget, T>)
	T& add(glm::vec2 size = {200.0f, 50.0f}, Args&&... args) {
		auto& t = push<T>(m_font, std::forward<Args>(args)...);
		t.m_rect.size = size;
		t.m_rect.anchor.offset = nextOffset();
		m_items.push_back(&t);
		return t;
	}

  private:
	glm::vec2 nextOffset() noexcept {
		if (!m_items.empty()) {
			Widget const& widget = *m_items.back();
			glm::vec2 offset{};
			switch (m_axis) {
			case Axis::eHorz: offset.x = widget.m_rect.size.x; break;
			case Axis::eVert: offset.y = -widget.m_rect.size.y; break;
			}
			return widget.m_rect.anchor.offset + offset;
		}
		return {};
	}

	std::vector<Widget*> m_items;
	Axis m_axis;
};
} // namespace le::gui
