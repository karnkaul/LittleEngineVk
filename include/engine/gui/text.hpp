#pragma once
#include <engine/gui/tree.hpp>
#include <graphics/mesh.hpp>
#include <graphics/text_factory.hpp>

namespace le {
class BitmapFont;
}

namespace le::gui {
class Text : public TreeNode {
  public:
	using Factory = graphics::TextFactory;

	Text(not_null<TreeRoot*> root, not_null<graphics::VRAM*> vram, not_null<BitmapFont const*> font) noexcept;

	Factory const& factory() const noexcept { return m_factory; }
	std::string_view str() const noexcept { return m_str; }
	void set(std::string str);
	void set(Factory factory);

	Span<Primitive const> primitives() const noexcept override;

	not_null<BitmapFont const*> m_font;

  private:
	void onUpdate(input::Space const&) override;

	graphics::Mesh m_mesh;
	Factory m_factory;
	std::string m_str;
	mutable Primitive m_prim;
	bool m_dirty = false;
};

// impl

inline Text::Text(not_null<TreeRoot*> root, not_null<graphics::VRAM*> vram, not_null<BitmapFont const*> font) noexcept
	: TreeNode(root), m_font(font), m_mesh(vram, graphics::Mesh::Type::eDynamic) {}
inline void Text::set(std::string str) {
	m_str = std::move(str);
	m_dirty = true;
}
inline void Text::set(Factory factory) {
	m_factory = std::move(factory);
	m_dirty = true;
}
} // namespace le::gui
