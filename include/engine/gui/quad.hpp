#pragma once
#include <engine/gui/tree.hpp>
#include <graphics/mesh.hpp>

namespace le::gui {
class Quad : public TreeNode {
  public:
	Quad(not_null<TreeRoot*> root, bool hitTest = true) noexcept;

	void onUpdate(input::Space const& space) override;
	Span<Prop const> props() const noexcept override;

	Material m_material;

  private:
	graphics::Mesh m_mesh;
	glm::vec2 m_size = {};
	mutable Prop m_prop;
};

// impl

inline Span<Prop const> Quad::props() const noexcept {
	m_prop = {m_material, &m_mesh};
	return m_prop;
}
} // namespace le::gui
