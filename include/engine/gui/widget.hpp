#pragma once
#include <core/time.hpp>
#include <engine/gui/quad.hpp>
#include <engine/gui/style.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/state.hpp>

namespace le::gui {
class Widget : public Quad {
  public:
	Widget(not_null<TreeRoot*> root, not_null<graphics::VRAM*> vram, not_null<BitmapFont const*> font);
	Widget(Widget&&) = delete;
	Widget& operator=(Widget&&) = delete;

	Status status(input::State const& state) const noexcept;
	bool clicked(input::State const& state, bool style = true) noexcept;

	struct {
		Style<Material> quad;
		Style<Colour> text;
	} m_styles;
	struct {
		time::Point point{};
		Status status = {};
		bool set = false;
	} m_previous;
	Time_ms m_debounce = 5ms;
	Text* m_text = {};
};
} // namespace le::gui
