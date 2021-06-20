#pragma once
#include <optional>
#include <graphics/mesh.hpp>
#include <graphics/text_factory.hpp>

namespace le {
namespace graphics {
class Texture;
}
struct Primitive;
class BitmapFont;

struct BitmapText {
	using Type = graphics::Mesh::Type;

	graphics::TextFactory text;
	std::optional<graphics::Mesh> mesh;

	void create(not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);
	bool set(BitmapFont const& font, std::string_view str);
	bool set(Span<graphics::Glyph const> glyphs, glm::ivec2 atlas, std::string_view str);
	Primitive primitive(BitmapFont const& font) const;
	Primitive primitive(graphics::Texture const& atlas) const;
};
} // namespace le
