#include <engine/gui/style.hpp>
#include <engine/gui/widgets/dropdown.hpp>

namespace le::gui {
namespace {
graphics::Geometry makeArrow(f32 scale, f32 z, graphics::RGBA colour) {
	graphics::Geometry geom;
	glm::vec3 const verts[] = {{-0.75f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.75f, 0.0f, 0.0f}};
	for (auto const vert : verts) {
		graphics::Vertex v{vert * scale, colour.toVec4()};
		v.position.z = z;
		geom.addVertex(v);
	}
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
	if (info.zIndex) { m_zIndex = *info.zIndex; }
	m_selected = info.selected;
	m_style.base.text.size = info.textSize;
	m_text->size(m_style.base.text.size);
	m_rect.size = info.size;
	m_textColours = info.textColours;
	m_cover = &push<Quad>(false);
	m_cover->m_material.Tf = info.coverColours.cover;
	m_cover->m_rect.anchor.norm.x = 0.5f;
	m_cover->m_rect.offset({m_rect.size.x * 0.12f, m_rect.size.y}, {-1.0f, 0.0f});
	m_arrow = &m_cover->push<Shape>();
	m_arrow->set(makeArrow(8.0f, m_zIndex, info.coverColours.arrow));
	info.flexbox.axis = Flexbox::Axis::eVert;
	m_flexbox = &push<Flexbox>(std::move(info.flexbox));
	f32 const yOffset = m_rect.size.y * 0.5f + m_rect.size.y * 0.5f + 0.5f; // +0.5f to round to next pixel
	m_flexbox->m_rect.anchor.offset = {0.0f, -yOffset};
	m_flexbox->m_active = false;
	m_onClickTk = onClick();
	m_onClickTk += [this]() {
		if (!m_flexbox->m_active) {
			expand();
		} else {
			collapse();
		}
	};
	std::string text = m_options[m_selected];
	itemPad(text, 0);
	m_text->set(std::move(text));
}

void Dropdown::add(Button& item, std::string_view text, std::size_t index) {
	item.m_text->set(std::string(text)).size(m_style.base.text.size).colour(m_style.base.text.colour).align(m_style.base.text.align);
	item.m_style = m_style;
	auto signal = item.onClick();
	signal += [this, index]() { select(index); };
	m_entrySignals.push_back(std::move(signal));
}

void Dropdown::expand() {
	m_flexbox->m_active = true;
	m_text->colour(m_textColours.expanded);
	refresh();
}

void Dropdown::collapse() {
	m_flexbox->m_active = false;
	m_text->colour(m_textColours.collapsed).set(m_options[m_selected]);
	refresh();
}

void Dropdown::select(std::size_t index) {
	m_selected = index;
	collapse();
	m_onSelect(selected());
}
} // namespace le::gui
