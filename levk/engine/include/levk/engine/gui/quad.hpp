#pragma once
#include <levk/engine/gui/tree.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le::gui {
class Quad : public TreeNode {
  public:
	Quad(not_null<TreeRoot*> root, bool hitTest = true) noexcept;

	void onUpdate(input::Space const& space) override;
	MeshView mesh() const noexcept override { return MeshObj{&m_primitive, &m_material}; }

	inline static u16 s_cornerPoints = 16;

	Material m_material;
	f32 m_cornerRadius = 0.0f;

  private:
	graphics::MeshPrimitive m_primitive;
	glm::vec2 m_size = {};
};
} // namespace le::gui
