#include <core/services.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept : TreeNode(root), m_mesh(Services::get<graphics::VRAM>()), m_font(font) {}

Text& Text::font(not_null<BitmapFont const*> font) {
	m_font = font;
	m_dirty = true;
	return *this;
}

Span<Prop const> Text::props() const noexcept {
	m_prop = m_gen.prop(m_mesh, m_font->atlas());
	return m_prop;
}

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	if (m_dirty) {
		write();
		m_dirty = false;
	}
}

void Text::write() {
	m_gen.position.z = m_zIndex;
	m_mesh.construct(m_gen(m_font->atlasSize(), m_font->glyphs(), m_str));
}
} // namespace le::gui
