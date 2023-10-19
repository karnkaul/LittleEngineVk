#include <le/scene/ui/button.hpp>

namespace le::ui {
Button::Button(Ptr<View> parent_view) : View(parent_view), m_quad(&push_element<Quad>()), m_text(&push_element<Text>()) {
	transform.extent = {200, 100};
	m_text->set_text("button");
	m_text->set_tint(graphics::black_v);
	m_text->set_align({.vert = VertAlign::eMid});
}

void Button::tick(Duration dt) {
	m_quad->transform.extent = transform.extent;
	View::tick(dt);
	auto const hitbox = graphics::Rect2D<>::from_extent(transform.extent, global_position());
	Interactable::update(hitbox);
}
} // namespace le::ui
