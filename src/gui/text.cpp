#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>

namespace le::gui {
View<Primitive> Text::primitives() const noexcept {
	if (m_font && m_mesh.valid()) {
		m_prim.material.map_Kd = &m_font->atlas();
		m_prim.material.map_d = &m_font->atlas();
		m_prim.mesh = &m_mesh;
		return m_prim;
	}
	return {};
}

void Text::onUpdate(input::Space const& space) {
	m_scissor = scissor(space, m_rect.origin, m_parent->m_rect.halfSize(), false);
	if (m_dirty && m_font) {
		m_factory.text = m_str;
		m_factory.pos.z = m_zIndex;
		m_mesh.construct(m_factory.generate(m_font->glyphs(), m_font->atlas().data().size));
		m_dirty = false;
	}
}
} // namespace le::gui
