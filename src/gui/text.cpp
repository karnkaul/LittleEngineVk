#include <core/services.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept : TreeNode(root), m_font(font) {
	m_text.make(Services::locate<graphics::VRAM>());
}

Text& Text::font(not_null<BitmapFont const*> font) {
	m_font = font;
	m_dirty = true;
	return *this;
}

Span<Prop const> Text::props() const noexcept { return m_text.prop(m_font->atlas()); }

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	if (m_dirty) {
		write();
		m_dirty = false;
	}
}

void Text::write() {
	m_text.position.z = m_zIndex;
	graphics::GlyphPen pen(&m_font->glyphs(), m_font->atlasSize(), m_text.size, m_text.position, m_text.colour);
	m_text.mesh->construct(pen.generate(m_str, m_text.align));
}
} // namespace le::gui
