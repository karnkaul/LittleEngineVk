#include <levk/engine/gui/widgets/flexbox.hpp>

namespace le::gui {
Flexbox::Flexbox(not_null<TreeRoot*> root, CreateInfo const& info) noexcept : gui::Widget(root, info.style), m_axis(info.axis), m_pad(info.pad) {
	m_style.widget.quad.reset();
	m_style.widget.quad.base = info.background;
	resize();
}

void Flexbox::setup(Widget& out_widget, glm::vec2 size, bool pad) {
	out_widget.m_rect.size = size;
	out_widget.m_rect.anchor.norm = m_axis == Axis::eVert ? glm::vec2(0.0f, 0.5f) : glm::vec2(-0.5f, 0.0f);
	out_widget.m_rect.anchor.offset = nextOffset();
	f32 const offset = pad ? m_pad : 0.0f;
	out_widget.m_rect.anchor.offset += m_axis == Axis::eVert ? glm::vec2(0.0f, -offset) : glm::vec2(offset, 0.0f);
	if (m_items.empty()) { out_widget.m_rect.anchor.offset += m_axis == Axis::eVert ? glm::vec2(0.0f, -0.5 * size.y) : glm::vec2(0.5 * size.x, 0.0f); }
	m_items.push_back(&out_widget);
	resize();
}

glm::vec2 Flexbox::nextOffset() const noexcept {
	if (!m_items.empty()) {
		Widget const& widget = *m_items.back();
		glm::vec2 offset{};
		switch (m_axis) {
		case Axis::eHorz: offset.x = widget.m_rect.size.x; break;
		case Axis::eVert: offset.y = -widget.m_rect.size.y; break;
		}
		return widget.m_rect.anchor.offset + offset;
	}
	return {};
}

void Flexbox::resize() noexcept {
	glm::vec2 size{};
	if (!m_items.empty()) {
		for (Widget const* widget : m_items) {
			size.y = std::max(size.y, widget->m_rect.size.y);
			size.x = std::max(size.x, widget->m_rect.size.x);
		}
		auto const& last = m_items.back()->m_rect;
		switch (m_axis) {
		case Axis::eHorz: size.x = std::abs(last.anchor.offset.x) + last.size.x * 0.5f + m_pad + 0.5f; break;
		case Axis::eVert: size.y = std::abs(last.anchor.offset.y) + last.size.y * 0.5f + m_pad + 0.5f; break;
		}
	}
	m_rect.size = size;
	m_rect.anchor.norm = m_axis == Axis::eVert ? glm::vec2(0.0f, -0.5f) : glm::vec2(0.5f, 0.0f);
	m_rect.anchor.offset = m_axis == Axis::eVert ? glm::vec2(0.0f, -0.5 * m_rect.size.y) : glm::vec2(0.5f * m_rect.size.x, 0.0f);
}
} // namespace le::gui
