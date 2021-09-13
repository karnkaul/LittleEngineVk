#pragma once
#include <engine/gui/widget.hpp>

namespace le::gui {
class Flexbox : public Widget {
  public:
	enum class Axis { eHorz, eVert };

	struct CreateInfo {
		Material background;
		Axis axis = Axis::eHorz;
		f32 pad = 5.0f;
		Hash style;
	};

	Flexbox(not_null<TreeRoot*> root, CreateInfo const& info) noexcept;

	template <typename T = Widget, typename... Args>
		requires(std::is_base_of_v<Widget, T>)
	T& add(glm::vec2 size = {200.0f, 50.0f}, bool pad = false, Args&&... args) {
		auto& t = push<T>(std::forward<Args>(args)...);
		setup(t, size, pad);
		return t;
	}

  private:
	void setup(Widget& out_widget, glm::vec2 size, bool pad);
	glm::vec2 nextOffset() const noexcept;
	void resize() noexcept;

	std::vector<Widget*> m_items;
	Axis m_axis;
	f32 m_pad{};
};
} // namespace le::gui
