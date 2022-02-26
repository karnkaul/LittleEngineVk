#include <levk/core/services.hpp>
#include <levk/gameplay/gui/shape.hpp>
#include <levk/graphics/draw_primitive.hpp>

namespace le::gui {
Shape::Shape(not_null<TreeRoot*> root) noexcept : TreeNode(root), m_primitive(Services::get<graphics::VRAM>(), graphics::MeshPrimitive::Type::eDynamic) {}

void Shape::addDrawPrimitives(DrawList& out) const {
	DrawPrimitive dp;
	dp.primitive = &m_primitive;
	dp.blinnPhong = &m_bpMaterial;
	pushPrimitive(out, dp);
}
} // namespace le::gui
