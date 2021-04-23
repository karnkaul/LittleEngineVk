#pragma once
#include <engine/gui/node.hpp>
#include <graphics/mesh.hpp>
#include <graphics/text_factory.hpp>

namespace le {
class BitmapFont;
}

namespace le::gui {
class Text : public Node {
  public:
	using Factory = graphics::TextFactory;

	Text(not_null<Root*> root, not_null<graphics::VRAM*> vram, not_null<BitmapFont const*> font) noexcept;

	void onUpdate(input::Space const&) override;
	View<Primitive> primitives() const noexcept override;

	std::string m_str;
	Factory m_text;
	not_null<BitmapFont const*> m_font;

  private:
	graphics::Mesh m_mesh;
	mutable Primitive m_prim;
};

// impl

inline Text::Text(not_null<Root*> root, not_null<graphics::VRAM*> vram, not_null<BitmapFont const*> font) noexcept
	: Node(root), m_font(font), m_mesh(vram, graphics::Mesh::Type::eDynamic) {
}
} // namespace le::gui
