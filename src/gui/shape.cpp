#include <core/services.hpp>
#include <engine/gui/shape.hpp>

namespace le::gui {
Shape::Shape(not_null<TreeRoot*> root) noexcept : TreeNode(root), m_mesh(Services::get<graphics::VRAM>(), graphics::Mesh::Type::eDynamic) {}
} // namespace le::gui
