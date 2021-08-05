#pragma once
#include <engine/gui/tree.hpp>
#include <graphics/mesh.hpp>

namespace le::gui {
class Shape : public TreeNode {
  public:
	Shape(not_null<TreeRoot*> root) noexcept;

	void set(graphics::Geometry geometry) { m_mesh.construct(std::move(geometry)); }
	Span<Primitive const> primitives() const noexcept override;

	Material m_material;

  private:
	graphics::Mesh m_mesh;
	mutable Primitive m_prim;
};

// impl

inline Span<Primitive const> Shape::primitives() const noexcept {
	if (m_mesh.valid()) {
		m_prim = {m_material, &m_mesh};
		return m_prim;
	}
	return {};
}
} // namespace le::gui
