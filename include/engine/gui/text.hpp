#pragma once
#include <engine/gui/tree.hpp>
#include <engine/render/bitmap_text.hpp>
#include <graphics/mesh.hpp>

namespace le {
class BitmapFont;
}

namespace le::gui {
class Text : public TreeNode {
  public:
	using Size = graphics::Glyph::Size;

	Text(not_null<TreeRoot*> root, not_null<BitmapFont const*> font) noexcept;

	std::string_view str() const noexcept { return m_str; }
	Text& set(std::string str);
	Text& size(Size size);
	Text& colour(graphics::RGBA colour);
	Text& position(glm::vec2 position);
	Text& align(glm::vec2 align);
	Text& font(not_null<BitmapFont const*> font);
	BitmapFont const* font() const noexcept { return m_font; }

	Span<Prop const> props() const noexcept override;

  private:
	void onUpdate(input::Space const& space) override;

	void write();

	graphics::Mesh m_mesh;
	TextGen m_gen;
	std::string m_str;
	mutable Prop m_prop;
	not_null<BitmapFont const*> m_font;
	bool m_dirty = false;
};

// impl

inline Text& Text::set(std::string str) {
	m_str = std::move(str);
	m_dirty = true;
	return *this;
}
inline Text& Text::size(Size size) {
	m_gen.size = size;
	m_dirty = true;
	return *this;
}
inline Text& Text::colour(graphics::RGBA colour) {
	m_gen.colour = colour;
	m_dirty = true;
	return *this;
}
inline Text& Text::position(glm::vec2 position) {
	m_gen.position = {position, m_zIndex};
	m_dirty = true;
	return *this;
}
inline Text& Text::align(glm::vec2 align) {
	m_gen.align = align;
	m_dirty = true;
	return *this;
}
} // namespace le::gui
