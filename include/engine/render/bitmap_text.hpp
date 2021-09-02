#pragma once
#include <optional>
#include <string>
#include <engine/scene/prop.hpp>
#include <graphics/glyph.hpp>
#include <graphics/mesh.hpp>

namespace le {
namespace graphics {
class Texture;
}
struct Prop;
class BitmapFont;

struct TextMesh {
	using Type = graphics::Mesh::Type;
	using Glyphs = graphics::GlyphMap;
	using Size = graphics::Glyph::Size;

	std::optional<graphics::Mesh> mesh;
	mutable Prop prop_;
	glm::vec3 position{};
	glm::vec2 align{};
	Size size;
	graphics::RGBA colour;

	void make(not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);
	bool set(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text);
	Span<Prop const> prop(graphics::Texture const& atlas) const;
};

class BitmapText {
  public:
	using Type = TextMesh::Type;

	BitmapText() = default;
	BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);

	bool set(std::string_view text);
	Span<Prop const> props() const;
	TextMesh& mesh() noexcept { return m_text; }

  private:
	TextMesh m_text;
	BitmapFont const* m_font;
};
} // namespace le
