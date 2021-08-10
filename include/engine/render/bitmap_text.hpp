#pragma once
#include <optional>
#include <string>
#include <engine/scene/primitive.hpp>
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

	std::optional<graphics::Mesh> mesh;
	mutable Primitive prim;
	graphics::TextFactory factory;

	void make(not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);
	bool set(BitmapFont const& font, std::string_view text);
	bool set(Span<graphics::Glyph const> glyphs, glm::ivec2 atlas, std::string_view text);
	Span<Primitive const> primitive(BitmapFont const& font) const;
	Span<Primitive const> primitive(graphics::Texture const& atlas) const;
};
} // namespace le
