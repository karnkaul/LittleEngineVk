#pragma once
#include <core/delegate.hpp>
#include <core/time.hpp>
#include <engine/gui/quad.hpp>
#include <engine/gui/style.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/state.hpp>

namespace le::gui {
class Widget : public Quad {
  public:
	using OnClick = Delegate<Widget&>;

	Widget(not_null<TreeRoot*> root, not_null<BitmapFont const*> font);
	Widget(Widget&&) = delete;
	Widget& operator=(Widget&&) = delete;

	Status status(input::State const& state) const noexcept;
	bool clicked(input::State const& state, bool style = true, Status* out = nullptr) noexcept;
	[[nodiscard]] OnClick::Tk onClick(OnClick::Callback const& cb) { return m_onClick.subscribe(cb); }
	void refresh(input::State const* state = nullptr);

	virtual Status onInput(input::State const& state);

	struct {
		Style<Material> quad;
		Style<graphics::RGBA> text;
	} m_styles;
	struct {
		time::Point point{};
		Status status = {};
		bool set = false;
	} m_previous;
	Time_ms m_debounce = 5ms;
	Text* m_text = {};

  protected:
	OnClick m_onClick;
	not_null<BitmapFont const*> m_font;

  private:
	bool clickedImpl(bool style, Status st) noexcept;
};
} // namespace le::gui
