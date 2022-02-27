#pragma once
#include <levk/gameplay/gui/tree.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le::gui {
class Quad : public TreeNode {
  public:
	Quad(not_null<TreeRoot*> root, bool hitTest = true) noexcept;

	void onUpdate(input::Space const& space) override;
	void addDrawPrimitives(DrawList& out) const override;

	inline static u16 s_cornerPoints = 16;

	graphics::BPMaterialData m_bpMaterial;
	f32 m_cornerRadius = 0.0f;

  private:
	graphics::MeshPrimitive m_primitive;
	glm::vec2 m_size = {};
};
} // namespace le::gui
