#include <spaced/resources/font_asset.hpp>
#include <spaced/resources/resources.hpp>
#include <spaced/scene/ui/text.hpp>

namespace spaced::ui {
Text::Text() {
	if (auto* font = Resources::self().load<FontAsset>(default_font_uri)) { m_font = &*font->font; }
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
		// NOLINTNEXTLINE
		case Align::eLeft: return -0.5f;
		// NOLINTNEXTLINE
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

auto Text::render_tree(std::vector<graphics::RenderObject>& out) const -> void {
	if (m_dirty) { refresh(); }

	render_to(out, &m_material, &m_primitive);

	Renderable::render_tree(out);
}
} // namespace spaced::ui
