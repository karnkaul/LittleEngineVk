#include <levk/core/services.hpp>
#include <levk/engine/input/space.hpp>
#include <levk/gameplay/gui/text.hpp>
#include <levk/graphics/font/font.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> const root, Hash const fontURI) noexcept : TreeNode(root), m_font(findFont(fontURI)), m_fontURI(fontURI), m_textMesh(m_font) {}

Text& Text::height(u32 const h) {
	m_textMesh.m_info.get<Line>().layout.scale = m_font->scale(h);
	return *this;
}

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	m_textMesh.m_info.get<Line>().layout.origin.z = m_zIndex;
}
} // namespace le::gui
