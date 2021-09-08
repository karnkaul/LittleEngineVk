#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/gui/widgets/input_field.hpp>

namespace le::gui {
InputField::InputField(not_null<TreeRoot*> root, not_null<BitmapFont const*> font, CreateInfo const& info, Hash style)
	: Widget(root, style), m_mesh(Services::get<graphics::VRAM>(), font), m_cursor(font) {
	m_rect.size = info.size;
	m_cursor.m_gen.colour = m_style.base.text.colour;
	m_cursor.m_gen.size = m_style.base.text.size;
	m_cursor.m_gen.position = {m_rect.size * m_cursor.m_gen.align, m_zIndex};
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
	m_props = {Quad::props().front(), m_mesh.prop()};
	if (auto cursor = m_cursor.props(); !cursor.empty()) { m_props.push_back(cursor.front()); }
	return m_props;
}

bool InputField::block(input::State const& state) {
	if (m_cursor.active()) {
		m_cursor.m_gen.position = {m_rect.size * m_cursor.m_gen.align, m_zIndex};
		graphics::Geometry geom;
		if (m_cursor.update(state, &geom)) { m_mesh.mesh.construct(std::move(geom)); }
		if (!m_cursor.active()) { setActive(false); }
		return true;
	}
	return false;
}

void InputField::setActive(bool active) noexcept {
	m_cursor.setActive(active);
	if (active) {
		Services::get<Engine>()->pushReceiver(this);
	} else {
		m_inputHandle = {};
	}
}
} // namespace le::gui
