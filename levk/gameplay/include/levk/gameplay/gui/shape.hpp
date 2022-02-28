#pragma once
#include <levk/gameplay/gui/tree.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le::gui {
class Shape : public TreeNode {
  public:
	Shape(not_null<TreeRoot*> root) noexcept;

	void set(graphics::Geometry geometry) { m_primitive.construct(std::move(geometry)); }
	void addDrawPrimitives(DrawList& out) const override;

	DrawPrimitive drawPrimitive() const;

	graphics::BPMaterialData m_bpMaterial;

  private:
	graphics::MeshPrimitive m_primitive;
};
} // namespace le::gui
