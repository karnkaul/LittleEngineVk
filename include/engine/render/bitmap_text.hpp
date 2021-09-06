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

struct TextGen {
	using Type = graphics::Mesh::Type;
	using Glyphs = graphics::GlyphMap;
	using Size = graphics::Glyph::Size;

	glm::vec3 position{};
	glm::vec2 align{};
	Size size = 1.0f;
	graphics::RGBA colour;

	static Prop prop(graphics::Mesh const& mesh, graphics::Texture const& atlas);

	graphics::Geometry operator()(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text) const;
};

class BitmapText {
  public:
	using Type = TextGen::Type;

	BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);

	virtual bool set(std::string_view text);
	virtual Span<Prop const> props() const;
	TextGen& gen() noexcept { return m_gen; }

  protected:
	graphics::Mesh m_mesh;
	TextGen m_gen;
	mutable Prop m_prop;
	not_null<BitmapFont const*> m_font;
};
} // namespace le
