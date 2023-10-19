#include <le/resources/font_asset.hpp>
#include <le/resources/resources.hpp>
#include <le/scene/ui/text.hpp>

namespace le::ui {
Text::Text(NotNull<View*> parent_view) : Renderable(parent_view) {
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
	m_text_dirty = true;
	return *this;
}

auto Text::refresh() -> void {
	if (m_font == nullptr) { return; }

	auto const n_offset = [&] {
		auto ret = glm::vec2{};
		switch (m_align.horz) {
		case HorzAlign::eLeft: ret.x = -0.5f; break;  // NOLINT
		case HorzAlign::eRight: ret.x = +0.5f; break; // NOLINT
		default: break;
		}
		switch (m_align.vert) {
		case VertAlign::eTop: ret.y = 0.5f; break;	   // NOLINT
		case VertAlign::eBottom: ret.y = -0.5f; break; // NOLINT
		default: break;
		}
		return ret;
	}();

	auto pen = graphics::Font::Pen{*m_font, m_height};
	auto const offset = (n_offset - 0.5f) * pen.calc_line_extent(m_text);
	pen.cursor.x = offset.x;
	pen.cursor.y = offset.y;
	m_text_start = pen.cursor;
	m_text_primitive.set_geometry(pen.generate_quads(m_text));
	m_text_material.texture = &pen.get_texture();

	m_text_dirty = false;
}

auto Text::tick(Duration /*dt*/) -> void {
	if (m_text_dirty) { refresh(); }
}

auto Text::render(std::vector<graphics::RenderObject>& out) const -> void { render_text(out); }

auto Text::render_text(std::vector<graphics::RenderObject>& out) const -> void { render_to(out, &m_text_material, &m_text_primitive); }
} // namespace le::ui
