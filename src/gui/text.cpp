#include <engine/gui/node.hpp>
#include <engine/gui/text.hpp>
#include <engine/input/space.hpp>
#include <engine/render/bitmap_font.hpp>

namespace le::gui {
View<Primitive> Text::primitives() const noexcept {
	if (m_font && !m_text.text.empty()) {
		m_prim.material.map_Kd = &m_font->atlas();
		m_prim.material.map_d = &m_font->atlas();
		m_prim.mesh = &m_mesh;
	}
	return m_prim;
}

void Text::onUpdate(input::Space const& space) {
	if (m_font) {
		auto const size = m_parent->m_size;
		glm::vec2 const hs = {size.x * 0.5f, -size.y * 0.5f};
		m_scissor = {space.screen(m_origin - hs, false), space.screen(m_origin + hs, false)};
		m_text.text = m_str;
		m_text.pos.z = m_zIndex;
		m_mesh.construct(m_text.generate(m_font->glyphs(), m_font->atlas().data().size));
	}
}
} // namespace le::gui
