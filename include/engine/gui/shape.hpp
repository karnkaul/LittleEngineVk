#pragma once
#include <engine/gui/tree.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le::gui {
class Shape : public TreeNode {
  public:
	Shape(not_null<TreeRoot*> root) noexcept;

	void set(graphics::Geometry geometry) { m_primitive.construct(std::move(geometry)); }
	Span<Prop const> props() const noexcept override;
	MeshView mesh() const noexcept override { return MeshObj{&m_primitive, &m_material}; }

	Material m_material;

  private:
	graphics::MeshPrimitive m_primitive;
	mutable Prop m_prop;
};

// impl

inline Span<Prop const> Shape::props() const noexcept {
	if (m_primitive.valid()) {
		m_prop = {&m_material, &m_primitive};
		return m_prop;
	}
	return {};
}
} // namespace le::gui
