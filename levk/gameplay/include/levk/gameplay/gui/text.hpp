#pragma once
#include <levk/engine/render/text_mesh.hpp>
#include <levk/gameplay/gui/tree.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le::gui {
class Text : public TreeNode {
  public:
	using Line = TextMesh::Line;
	using Font = graphics::Font;

	Text(not_null<TreeRoot*> root, Hash fontURI = defaultFontURI) noexcept;

	std::string_view str() const noexcept { return m_textMesh.m_info.get<Line>().line; }
	Text& set(std::string str);
	Text& height(u32 h) { return (m_height = h, *this); }
	Text& colour(graphics::RGBA colour);
	Text& position(glm::vec2 position);
	Text& align(glm::vec2 pivot);

	MeshView mesh() const noexcept override { return m_textMesh.mesh(); }
	void addDrawPrimitives(DrawList& out) const override;

	Hash m_fontURI = defaultFontURI;

  private:
	void onUpdate(input::Space const& space) override;

	mutable TextMesh m_textMesh;
	std::optional<u32> m_height;
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
} // namespace le::gui
