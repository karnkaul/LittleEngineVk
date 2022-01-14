#include <core/maths.hpp>
#include <core/services.hpp>
#include <levk/engine/gui/quad.hpp>

namespace le::gui {
Quad::Quad(not_null<TreeRoot*> root, bool hitTest) noexcept
	: TreeNode(root), m_primitive(Services::get<graphics::VRAM>(), graphics::MeshPrimitive::Type::eDynamic) {
	m_hitTest = hitTest;
}

void Quad::onUpdate(input::Space const&) {
	if (!maths::equals(glm::length2(m_rect.size), glm::length2(m_size), 0.25f)) {
		m_size = m_rect.size;
		graphics::GeomInfo gi;
		gi.origin.z = m_zIndex;
		m_primitive.construct(graphics::makeRoundedQuad(m_size, m_cornerRadius, s_cornerPoints, gi));
	}
}
} // namespace le::gui
