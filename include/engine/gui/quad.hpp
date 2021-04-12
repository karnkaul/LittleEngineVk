#pragma once
#include <engine/gui/node.hpp>
#include <graphics/mesh.hpp>

namespace le::gui {
class Quad : public Node {
  public:
	Quad(Root& root, graphics::VRAM& vram) noexcept;

	void onUpdate() override;
	View<Primitive> primitives() const noexcept override;

	Material m_material;

  private:
	graphics::Mesh m_mesh;
	mutable Primitive m_prim;
};

// impl

inline Quad::Quad(Root& root, graphics::VRAM& vram) noexcept : Node(root), m_mesh(vram, graphics::Mesh::Type::eDynamic) {
}
inline View<Primitive> Quad::primitives() const noexcept {
	m_prim = {m_material, &m_mesh};
	return m_prim;
}
} // namespace le::gui
