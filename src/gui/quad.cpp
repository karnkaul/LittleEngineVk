#include <engine/gui/quad.hpp>

namespace le::gui {
void Quad::onUpdate(input::Space const&) { m_mesh.construct(graphics::makeQuad(m_rect.size)); }
} // namespace le::gui
