#include <engine/gui/dropdown.hpp>

namespace le::gui {
bool Dropdown::itemPad(std::string& out_text, std::size_t index) const noexcept {
	if (index == 0) { return true; }
	if (out_text.size() > 2 && out_text[0] == '/') {
		switch (out_text[1]) {
		case 'b': {
			out_text = out_text.substr(2);
			return true;
		}
		default: break;
		}
	}
	return false;
}

void Dropdown::init(CreateInfoBase info) {
	m_selected = info.selected;
	m_styles.quad = info.quadStyle;
	m_rect.size = info.size;
	m_textColours = info.textColours;
	m_cover = &push<Quad>(false);
	m_cover->m_material.Tf = m_textColours.expanded;
	m_cover->m_rect.anchor.norm.x = 0.5f;
	m_cover->m_rect.offset({m_rect.size.x * 0.12f, m_rect.size.y}, {-1.0f, 0.0f});
	info.flexbox.axis = Flexbox::Axis::eVert;
	m_flexbox = &push<Flexbox>(m_font, std::move(info.flexbox));
	f32 const yOffset = m_rect.size.y * 0.5f + m_rect.size.y * 0.5f + 0.5f; // +0.5f to round to next pixel
	m_flexbox->m_rect.anchor.offset = {0.0f, -yOffset};
	m_flexbox->m_active = false;
	m_onClickTk = onClick([this](Widget&) {
		if (!m_flexbox->m_active) {
			expand();
		} else {
			collapse();
		}
	});
	m_text->set(m_options[m_selected], m_textFactory);
}

void Dropdown::add(Widget& item, std::string_view text, std::size_t index) {
	item.m_text->set(std::string(text), m_textFactory);
	item.m_styles.quad = m_styles.quad;
	m_entryTokens.push_back(item.onClick([this, index](Widget&) { select(index); }));
}

void Dropdown::expand() {
	m_flexbox->m_active = true;
	m_styles.text.base = m_textColours.expanded;
	refresh();
}

void Dropdown::collapse() {
	m_flexbox->m_active = false;
	m_styles.text.base = m_textColours.collapsed;
	m_text->set(m_options[m_selected], m_textFactory);
	refresh();
}

void Dropdown::select(std::size_t index) {
	m_selected = index;
	collapse();
	m_onSelect(*this, selected());
}
} // namespace le::gui
