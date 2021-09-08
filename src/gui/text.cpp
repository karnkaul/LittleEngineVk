#include <core/services.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept : TreeNode(root), m_font(font), m_mesh(Services::get<graphics::VRAM>(), font) {}

Text& Text::font(not_null<BitmapFont const*> font) {
	m_font = m_mesh.font = font;
	m_dirty = true;
	return *this;
}

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	if (m_dirty) {
		write();
		m_dirty = false;
	}
}

void Text::write() {
	m_mesh.gen.position.z = m_zIndex;
	m_mesh.mesh.construct(m_mesh.gen(m_font->atlasSize(), m_font->glyphs(), m_str));
}
} // namespace le::gui
