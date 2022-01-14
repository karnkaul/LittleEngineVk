#pragma once
#include <graphics/mesh_primitive.hpp>
#include <levk/engine/gui/tree.hpp>
#include <levk/engine/render/text_mesh.hpp>

namespace le::gui {
class Text : public TreeNode {
  public:
	using Line = TextMesh::Line;
	using Font = graphics::Font;

	Text(not_null<TreeRoot*> root, Hash fontURI = defaultFontURI) noexcept;

	std::string_view str() const noexcept { return m_textMesh.m_info.get<Line>().line; }
	Text& set(std::string str);
	Text& height(u32 h);
	Text& colour(graphics::RGBA colour);
	Text& position(glm::vec2 position);
	Text& align(glm::vec2 pivot);
	Text& font(not_null<Font*> font);
	Font const& font() const noexcept { return *m_font; }

	MeshView mesh() const noexcept override { return m_textMesh.mesh(); }

	not_null<Font*> m_font;
	Hash m_fontURI = defaultFontURI;

  private:
	void onUpdate(input::Space const& space) override;

	TextMesh m_textMesh;
};

// impl

inline Text& Text::set(std::string str) {
	m_textMesh.m_info.get<Line>().line = std::move(str);
	return *this;
}
inline Text& Text::colour(graphics::RGBA const colour) {
	m_textMesh.m_colour = colour;
	return *this;
}
inline Text& Text::position(glm::vec2 const position) {
	m_textMesh.m_info.get<Line>().layout.origin = {position, m_zIndex};
	return *this;
}
inline Text& Text::align(glm::vec2 const pivot) {
	m_textMesh.m_info.get<Line>().layout.pivot = pivot;
	return *this;
}
inline Text& Text::font(not_null<Font*> const font) {
	m_font = font;
	return *this;
}
} // namespace le::gui
