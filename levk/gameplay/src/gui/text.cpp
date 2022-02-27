#include <levk/core/services.hpp>
#include <levk/engine/input/space.hpp>
#include <levk/gameplay/gui/text.hpp>
#include <levk/graphics/font/font.hpp>
#include <levk/graphics/render/draw_list.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> const root, Hash const fontURI) noexcept : TreeNode(root), m_fontURI(fontURI), m_textMesh(vram()) {}

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	if (auto font = findFont(m_fontURI)) {
		m_textMesh.font(font);
		if (m_height) { m_textMesh.m_info.get<Line>().layout.scale = font->scale(*m_height); }
	}
	m_textMesh.m_info.get<Line>().layout.origin.z = m_zIndex;
}

void Text::addDrawPrimitives(DrawList& out) const { pushDrawPrimitives(out, drawPrimitive()); }
graphics::DrawPrimitive Text::drawPrimitive() const { return m_textMesh.drawPrimitive(); }
} // namespace le::gui
