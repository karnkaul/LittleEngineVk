#include <levk/core/services.hpp>
#include <levk/engine/gui/shape.hpp>

namespace le::gui {
Shape::Shape(not_null<TreeRoot*> root) noexcept : TreeNode(root), m_primitive(Services::get<graphics::VRAM>(), graphics::MeshPrimitive::Type::eDynamic) {}
} // namespace le::gui
