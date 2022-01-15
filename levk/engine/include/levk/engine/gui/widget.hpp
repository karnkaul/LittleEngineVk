#pragma once
#include <ktl/delegate.hpp>
#include <levk/core/time.hpp>
#include <levk/engine/gui/quad.hpp>
#include <levk/engine/gui/style.hpp>
#include <levk/engine/input/state.hpp>

namespace le::gui {
class Widget : public Quad {
  public:
	using Status = InteractStatus;
	using OnClick = ktl::delegate<>;

	Widget(not_null<TreeRoot*> root, Hash style = {});
	Widget(Widget&&) = delete;
	Widget& operator=(Widget&&) = delete;

	Status status(input::State const& state) const noexcept;
	bool clicked(input::State const& state, bool style = true, Status* out = nullptr) noexcept;
	[[nodiscard]] OnClick::handle onClick() { return m_onClick.make_signal(); }
	void refresh(input::State const* state = nullptr);

	virtual Status onInput(input::State const& state);

	Style m_style;
	struct {
		time::Point point{};
		Status status = {};
		bool set = false;
	} m_previous;
	Time_ms m_debounce = 5ms;
	bool m_interact = true;

  protected:
	OnClick m_onClick;

  private:
	bool clickedImpl(bool style, Status st) noexcept;
};
} // namespace le::gui
