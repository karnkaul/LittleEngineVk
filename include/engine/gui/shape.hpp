#pragma once
#include <engine/gui/tree.hpp>
#include <graphics/mesh.hpp>

namespace le::gui {
class Shape : public TreeNode {
  public:
	Shape(not_null<TreeRoot*> root) noexcept;

	void set(graphics::Geometry geometry) { m_mesh.construct(std::move(geometry)); }
	Span<Prop const> props() const noexcept override;

	Material m_material;

  private:
	graphics::Mesh m_mesh;
	mutable Prop m_prop;
};

// impl

inline Span<Prop const> Shape::props() const noexcept {
	if (m_mesh.valid()) {
		m_prop = {m_material, &m_mesh};
		return m_prop;
	}
	return {};
}
} // namespace le::gui
