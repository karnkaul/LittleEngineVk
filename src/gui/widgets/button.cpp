#include <engine/gui/widgets/button.hpp>

namespace le::gui {
Button::Button(not_null<TreeRoot*> parent, Hash fontURI, Hash style) : Widget(parent, style) {
	m_text = &push<Text>(fontURI);
	m_text->size(m_style.base.text.size).colour(m_style.base.text.colour).align(m_style.base.text.align);
}
} // namespace le::gui
