#include <engine/gui/dropdown.hpp>

namespace le::gui {
void Dropdown::init(Style<Material> style, glm::vec2 size, std::string const& text, f32 entryScale) {
	m_styles.quad = style;
	m_rect.size = size;
	m_flexbox = &push<Flexbox>(m_font, Flexbox::Axis::eVert);
	f32 const yOffset = m_rect.size.y * 0.5f + m_rect.size.y * entryScale * 0.5f + 0.5f; // +0.5f to round to next pixel
	m_flexbox->m_rect.anchor.offset = {0.0f, -yOffset};
	m_flexbox->m_active = false;
	m_onClickTk = onClick([this](Widget&) {
		if (!m_flexbox->m_active) {
			expand();
		} else {
			collapse();
		}
	});
	m_text->set(text, m_textFactory);
}

void Dropdown::add(Widget& item, std::string const& text, std::size_t index) {
	item.m_text->set(text, m_textFactory);
	item.m_styles.quad = m_styles.quad;
	m_entryTokens.push_back(item.onClick([this, index](Widget&) { select(index); }));
}

void Dropdown::expand() {
	m_flexbox->m_active = true;
	m_text->m_active = false;
}

void Dropdown::collapse() {
	m_flexbox->m_active = false;
	m_text->m_active = true;
	m_text->set(m_options[m_selected], m_textFactory);
	refresh();
}

void Dropdown::select(std::size_t index) {
	m_selected = index;
	collapse();
	m_onSelect(*this, selected());
}
} // namespace le::gui
