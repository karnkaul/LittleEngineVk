#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/gui/widgets/input_field.hpp>

namespace le::gui {
InputField::InputField(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, CreateInfo const& info, Hash style)
	: Widget(root, style), m_textMesh(Services::get<graphics::VRAM>(), font), m_cursor(font), m_secret(info.secret) {
	m_rect.size = info.size;
	m_outline = Quad(this);
	m_outline->m_rect.size = info.size + 5.0f;
	m_outline->m_material.Tf = m_style.base.text.colour;
	m_outline->m_material.d = m_cursor.m_alpha = info.alpha;
	m_offsetX = info.offsetX;
	m_outline->update({});
	m_cursor.m_gen.colour = m_style.base.text.colour;
	m_cursor.m_gen.size = m_style.base.text.size;
	reposition();
	m_cursor.m_flags.assign(input::TextCursor::Flag::eNoNewLine, !info.multiLine);
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

Span<Prop const> InputField::props() const noexcept {
	m_props = {m_outline->props().front(), Quad::props().front(), m_textMesh.prop()};
	if (auto cursor = m_cursor.props(); !cursor.empty()) { m_props.push_back(cursor.front()); }
	return m_props;
}

bool InputField::block(input::State const& state) {
	if (m_cursor.active()) {
		reposition();
		graphics::Geometry geom;
		if (m_cursor.update(state, m_secret ? nullptr : &geom)) {
			if (m_secret) {
				m_exposed = std::move(m_cursor.m_text);
				m_cursor.m_text = std::string(m_exposed.size(), '*');
				geom = {};
				m_cursor.refresh(&geom);
				m_textMesh.primitive.construct(std::move(geom));
			} else {
				m_textMesh.primitive.construct(std::move(geom));
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
	m_cursor.m_gen.position = {size * m_cursor.m_gen.align, m_zIndex};
}
} // namespace le::gui
