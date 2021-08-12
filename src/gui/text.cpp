#include <core/services.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>

namespace le::gui {
Text::Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept : TreeNode(root), m_font(font) {
	m_text.make(Services::locate<graphics::VRAM>());
	m_text.set(*m_font, "");
}

Span<Prop const> Text::props() const noexcept { return m_text.prop(*m_font); }

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	if (m_dirty && m_font) {
		m_text.factory.pos.z = m_zIndex;
		m_text.set(*m_font, m_str);
		m_dirty = false;
	}
}
} // namespace le::gui
