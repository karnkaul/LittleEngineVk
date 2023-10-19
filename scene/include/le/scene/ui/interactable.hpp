#pragma once
#include <le/core/polymorphic.hpp>
#include <le/core/time.hpp>
#include <le/graphics/rect.hpp>

namespace le::ui {
class Interactable : public Polymorphic {
  public:
	[[nodiscard]] auto in_focus() const -> bool { return m_in_focus; }

	void update(graphics::Rect2D<> const& hitbox);

  protected:
	virtual void on_focus_gained() {}
	virtual void on_focus_lost() {}
	virtual void on_press() {}
	virtual void on_release() {}

  private:
	bool m_in_focus{};
};
} // namespace le::ui
