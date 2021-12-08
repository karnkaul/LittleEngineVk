#include <core/services.hpp>
#include <engine/gui/shape.hpp>

namespace le::gui {
Shape::Shape(not_null<TreeRoot*> root) noexcept : TreeNode(root), m_primitive(Services::get<graphics::VRAM>(), graphics::MeshPrimitive::Type::eDynamic) {}
} // namespace le::gui
