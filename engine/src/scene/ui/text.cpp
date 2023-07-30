#include <spaced/resources/font_asset.hpp>
#include <spaced/resources/resources.hpp>
#include <spaced/scene/ui/text.hpp>

namespace spaced::ui {
Text::Text() {
	if (auto* font = Resources::self().load<FontAsset>(default_font)) { m_font = &*font->font; }
}

auto Text::set_font(NotNull<graphics::Font*> font) -> Text& {
	m_font = font;
	return set_dirty();
}

auto Text::set_text(std::string text) -> Text& {
	m_text = std::move(text);
	return set_dirty();
}

auto Text::set_height(graphics::TextHeight height) -> Text& {
	m_height = graphics::clamp(height);
	return set_dirty();
}

auto Text::set_align(Align align) -> Text& {
	m_align = align;
	return set_dirty();
}

auto Text::set_dirty() -> Text& {
	m_dirty = true;
	return *this;
}

auto Text::refresh() const -> void {
	if (m_font == nullptr) { return; }

	auto const n_offset = [&] {
		switch (m_align) {
		case Align::eLeft: return -0.5f;
		case Align::eRight: return +0.5f;
		default: return 0.0f;
		}
	}();

	auto pen = graphics::Font::Pen{*m_font, m_height};
	auto const offset_x = (n_offset - 0.5f) * pen.calc_line_extent(m_text).x;
	pen.cursor.x = offset_x;
	m_primitive.set_geometry(pen.generate_quads(m_text));
	m_material.texture = &pen.get_texture();

	m_dirty = false;
}

auto Text::render_tree(Rect2D<> const& parent_frame, std::vector<graphics::RenderObject>& out) const -> void {
	if (m_dirty) { refresh(); }

	auto const obj = graphics::RenderObject{
		.material = &m_material,
		.primitive = &m_primitive,
		.parent = transform.matrix(),
		.pipeline_state = PrimitiveRenderer::pipeline_state_v,
	};
	out.push_back(obj);

	View::render_tree(parent_frame, out);
}
} // namespace spaced::ui
