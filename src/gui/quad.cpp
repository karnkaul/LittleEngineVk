#include <engine/gui/node.hpp>
#include <engine/gui/quad.hpp>

namespace le::gui {
void Quad::onUpdate(input::Space const&) {
	m_mesh.construct(graphics::makeQuad(m_size));
}
} // namespace le::gui
