#pragma once
#include <engine/gui/widget.hpp>

namespace le::gui {
class Flexbox : public Widget {
  public:
	enum class Axis { eHorz, eVert };

	struct CreateInfo {
		Material background;
		Axis axis = Axis::eHorz;
	};

	Flexbox(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, CreateInfo const& info) noexcept;

	template <typename T = Widget, typename... Args>
		requires(std::is_base_of_v<Widget, T>)
	T& add(glm::vec2 size = {200.0f, 50.0f}, f32 offset = 0.0f, Args&&... args) {
		auto& t = push<T>(m_font, std::forward<Args>(args)...);
		setup(t, size, offset);
		return t;
	}

  private:
	void setup(Widget& out_widget, glm::vec2 size, f32 offset);
	glm::vec2 nextOffset() const noexcept;
	void resize() noexcept;

	std::vector<Widget*> m_items;
	Axis m_axis;
};
} // namespace le::gui
