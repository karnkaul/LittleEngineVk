#pragma once
#include <optional>
#include <string>
#include <engine/scene/prop.hpp>
#include <graphics/glyph_pen.hpp>
#include <graphics/mesh.hpp>

namespace le {
namespace graphics {
class Texture;
}
struct Prop;
class BitmapFont;

struct BitmapTextMesh {
	using Type = graphics::Mesh::Type;
	using Glyphs = graphics::BitmapGlyphArray;

	std::optional<graphics::Mesh> mesh;
	mutable Prop prop_;
	glm::vec3 position{};
	glm::vec2 align{};
	graphics::BitmapGlyphPen::Size size;
	graphics::RGBA colour;

	void make(not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);
	bool set(BitmapFont const& font, Glyphs const& glyphs, std::string_view text);
	Span<Prop const> prop(BitmapFont const& font) const;
	Span<Prop const> prop(graphics::Texture const& atlas) const;
};

class BitmapText {
  public:
	using Type = BitmapTextMesh::Type;

	BitmapText() = default;
	BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);

	bool set(std::string_view text);
	Span<Prop const> props() const;
	BitmapTextMesh& mesh() noexcept { return m_text; }

  private:
	graphics::BitmapGlyphArray m_glyphs;
	BitmapTextMesh m_text;
	BitmapFont const* m_font;
};
} // namespace le
