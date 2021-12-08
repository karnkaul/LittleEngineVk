#pragma once
#include <engine/gui/text.hpp>
#include <engine/gui/widget.hpp>

namespace le::gui {
class Button : public Widget {
  public:
	Button(not_null<TreeRoot*> parent, Hash fontURI = defaultFontURI, Hash style = {});

	Button(Button&&) = delete;
	Button& operator=(Button&&) = delete;

	Text* m_text{};
};
} // namespace le::gui
