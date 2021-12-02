#pragma once
#include <engine/gui/tree.hpp>
#include <engine/render/material.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le::gui {
class Shape : public TreeNode {
  public:
	Shape(not_null<TreeRoot*> root) noexcept;

	void set(graphics::Geometry geometry) { m_primitive.construct(std::move(geometry)); }
	MeshView mesh() const noexcept override { return MeshObj{&m_primitive, &m_material}; }

	Material m_material;

  private:
	graphics::MeshPrimitive m_primitive;
};
} // namespace le::gui
