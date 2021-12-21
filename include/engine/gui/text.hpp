#pragma once
#include <engine/gui/tree.hpp>
#include <engine/render/bitmap_text.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le::gui {
class Text : public TreeNode {
  public:
	using Size = graphics::BitmapGlyph::Size;

	Text(not_null<TreeRoot*> root, Hash fontURI = defaultFontURI) noexcept;

	std::string_view str() const noexcept { return m_str; }
	Text& set(std::string str);
	Text& size(Size size);
	Text& colour(graphics::RGBA colour);
	Text& position(glm::vec2 position);
	Text& align(glm::vec2 align);
	Text& font(not_null<BitmapFont const*> font);
	BitmapFont const* font() const noexcept { return m_font; }

	MeshView mesh() const noexcept override { return m_textMesh.mesh(); }

	not_null<BitmapFont const*> m_font;
	Hash m_fontURI = defaultFontURI;

  private:
	void onUpdate(input::Space const& space) override;

	void write();

	TextMesh m_textMesh;
	std::string m_str;
	bool m_dirty = false;
};

// impl

inline Text& Text::set(std::string str) {
	m_str = std::move(str);
	m_dirty = true;
	return *this;
}
inline Text& Text::size(Size size) {
	m_textMesh.gen.size = size;
	m_dirty = true;
	return *this;
}
inline Text& Text::colour(graphics::RGBA colour) {
	m_textMesh.gen.colour = colour;
	m_dirty = true;
	return *this;
}
inline Text& Text::position(glm::vec2 position) {
	m_textMesh.gen.position = {position, m_zIndex};
	m_dirty = true;
	return *this;
}
inline Text& Text::align(glm::vec2 align) {
	m_textMesh.gen.align = align;
	m_dirty = true;
	return *this;
}
} // namespace le::gui
