#include <core/maths.hpp>
#include <core/services.hpp>
#include <engine/gui/quad.hpp>

namespace le::gui {
Quad::Quad(not_null<TreeRoot*> root, bool hitTest) noexcept : TreeNode(root), m_mesh(Services::locate<graphics::VRAM>(), graphics::Mesh::Type::eDynamic) {
	m_hitTest = hitTest;
}

void Quad::onUpdate(input::Space const&) {
	if (!maths::equals(glm::length2(m_rect.size), glm::length2(m_size), 0.25f)) {
		m_size = m_rect.size;
		m_mesh.construct(graphics::makeRoundedQuad(m_size, m_cornerRadius, s_cornerPoints));
	}
}
} // namespace le::gui
