#pragma once
#include <engine/gui/tree.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le::gui {
class Quad : public TreeNode {
  public:
	Quad(not_null<TreeRoot*> root, bool hitTest = true) noexcept;

	void onUpdate(input::Space const& space) override;
	Span<Prop const> props() const noexcept override;

	inline static u16 s_cornerPoints = 16;

	Material m_material;
	f32 m_cornerRadius = 0.0f;

  private:
	graphics::MeshPrimitive m_primitive;
	glm::vec2 m_size = {};
	mutable Prop m_prop;
};

// impl

inline Span<Prop const> Quad::props() const noexcept {
	m_prop = {m_material, &m_primitive};
	return m_prop;
}
} // namespace le::gui
