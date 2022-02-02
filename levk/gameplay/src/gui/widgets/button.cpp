#include <levk/gameplay/gui/widgets/button.hpp>

namespace le::gui {
Button::Button(not_null<TreeRoot*> parent, Hash fontURI, Hash style) : Widget(parent, style) {
	m_text = &push<Text>(fontURI);
	m_text->height(m_style.base.text.height).colour(m_style.base.text.colour).align(m_style.base.text.pivot);
}
} // namespace le::gui
