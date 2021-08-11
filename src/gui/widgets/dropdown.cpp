#include <engine/gui/style.hpp>
#include <engine/gui/widgets/dropdown.hpp>

namespace le::gui {
namespace {
graphics::Geometry makeArrow(f32 scale, graphics::RGBA colour) {
	graphics::Geometry geom;
	static constexpr glm::vec3 verts[] = {{-0.75f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.75f, 0.0f, 0.0f}};
	for (auto const vert : verts) { geom.addVertex(graphics::Vertex{vert * scale, colour.toVec4()}); }
	return geom;
}
} // namespace

bool Dropdown::itemPad(std::string& out_text, std::size_t index) const noexcept {
	if (out_text.size() > 2 && out_text[0] == '/') {
		switch (out_text[1]) {
		case 'b': {
			out_text = out_text.substr(2);
			return true;
		}
		default: break;
		}
	}
	if (index == 0) { return true; }
	return false;
}

void Dropdown::init(CreateInfoBase info) {
	m_selected = info.selected;
	m_style.text.base.size = info.textSize;
	m_rect.size = info.size;
	m_textColours = info.textColours;
	m_cover = &push<Quad>(false);
	m_cover->m_material.Tf = info.coverColours.cover;
	m_cover->m_rect.anchor.norm.x = 0.5f;
	m_cover->m_rect.offset({m_rect.size.x * 0.12f, m_rect.size.y}, {-1.0f, 0.0f});
	m_arrow = &m_cover->push<Shape>();
	m_arrow->set(makeArrow(8.0f, info.coverColours.arrow));
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
	std::string text = m_options[m_selected];
	itemPad(text, 0);
	m_text->set(std::move(text));
}

void Dropdown::add(Widget& item, std::string_view text, std::size_t index) {
	item.m_text->set(std::string(text));
	item.m_style = m_style;
	m_entryTokens.push_back(item.onClick([this, index](Widget&) { select(index); }));
}

void Dropdown::expand() {
	m_flexbox->m_active = true;
	m_style.text.base.colour = m_textColours.expanded;
	refresh();
}

void Dropdown::collapse() {
	m_flexbox->m_active = false;
	m_style.text.base.colour = m_textColours.collapsed;
	m_text->set(m_options[m_selected]);
	refresh();
}

void Dropdown::select(std::size_t index) {
	m_selected = index;
	collapse();
	m_onSelect(*this, selected());
}
} // namespace le::gui
