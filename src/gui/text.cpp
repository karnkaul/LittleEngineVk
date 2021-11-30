#include <core/services.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>
#include <graphics/glyph_pen.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept
	: TreeNode(root), m_font(font), m_textMesh(Services::get<graphics::VRAM>(), font) {}

Text& Text::font(not_null<BitmapFont const*> font) {
	m_font = m_textMesh.font = font;
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
	m_textMesh.gen.position.z = m_zIndex;
	m_textMesh.primitive.construct(m_textMesh.gen(m_font->atlasSize(), m_font->glyphs(), m_str));
}
} // namespace le::gui
