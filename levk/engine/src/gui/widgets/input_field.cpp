#include <levk/core/services.hpp>
#include <levk/core/utils/enumerate.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/gui/widgets/input_field.hpp>
#include <levk/graphics/font/font.hpp>

namespace le::gui {
InputField::InputField(not_null<TreeRoot*> root, CreateInfo const& info, Hash fontURI, Hash style)
	: Widget(root, style), m_font(findFont(fontURI)), m_textMesh(m_font), m_cursor(m_font), m_secret(info.secret) {
	m_rect.size = info.size;
	m_outline = Quad(this);
	m_outline->m_rect.size = info.size + 5.0f;
	m_outline->m_material.Tf = m_style.base.text.colour;
	m_outline->m_material.d = m_cursor.m_alpha = info.alpha;
	m_offsetX = info.offsetX;
	m_outline->update({});
	m_cursor.m_colour = m_style.base.text.colour;
	m_cursor.m_layout.scale = m_font->scale(m_style.base.text.height);
	align(Font::Align::eMin);
	setActive(info.active);
}

InputField::Status InputField::onInput(input::State const& state) {
	auto const ret = Widget::onInput(state);
	if (listening()) {
		if (state.pressed(input::Key::eMouseButton1) && !hit(state.cursor.position)) { setActive(false); }
	} else {
		if (ret == Status::eRelease) { setActive(true); }
	}
	return ret;
}

MeshView InputField::mesh() const noexcept {
	m_meshes = {m_outline->mesh().front(), Quad::mesh().front()};
	if (auto text = m_textMesh.mesh(); !text.empty()) { m_meshes.push_back(text.front()); }
	if (auto cursor = m_cursor.mesh(); !cursor.empty()) { m_meshes.push_back(cursor.front()); }
	return MeshObjView(m_meshes);
}

bool InputField::block(input::State const& state) {
	if (m_cursor.active()) {
		reposition();
		graphics::Geometry geom;
		if (m_cursor.update(state, m_secret ? nullptr : &geom)) {
			if (m_secret) {
				m_exposed = std::move(m_cursor.m_line);
				m_cursor.m_line = std::string(m_exposed.size(), '*');
				geom = {};
				m_cursor.refresh(&geom);
				m_textMesh.m_info = std::move(geom);
			} else {
				m_textMesh.m_info = std::move(geom);
			}
		}
		if (!m_cursor.active()) { setActive(false); }
		return true;
	}
	return false;
}

void InputField::setActive(bool active) noexcept {
	m_cursor.setActive(active);
	if (active) {
		Services::get<Engine>()->pushReceiver(this);
		reposition();
		m_cursor.refresh();
	} else {
		m_inputHandle = {};
	}
}

void InputField::onUpdate(input::Space const& space) {
	Widget::onUpdate(space);
	m_outline->update(space);
}

void InputField::reposition() noexcept {
	glm::vec2 const size = {m_rect.size.x - m_offsetX, m_rect.size.y};
	m_cursor.m_layout.origin = {size * m_cursor.m_layout.pivot, m_zIndex};
	m_cursor.m_layout.origin.y += size.x * 0.03f;
}
} // namespace le::gui
