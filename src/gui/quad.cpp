#include <core/maths.hpp>
#include <engine/gui/quad.hpp>

namespace le::gui {
void Quad::onUpdate(input::Space const&) {
	if (!maths::equals(glm::length2(m_rect.size), glm::length2(m_size), 0.25f)) {
		m_size = m_rect.size;
		m_mesh.construct(graphics::makeQuad(m_size));
	}
}
} // namespace le::gui
