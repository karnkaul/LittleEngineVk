#pragma once
#include <optional>
#include <string>
#include <engine/scene/prop.hpp>
#include <graphics/mesh.hpp>
#include <graphics/text_factory.hpp>

namespace le {
namespace graphics {
class Texture;
}
struct Prop;
class BitmapFont;

struct BitmapText {
	using Type = graphics::Mesh::Type;

	std::optional<graphics::Mesh> mesh;
	mutable Prop prop_;
	graphics::TextFactory factory;

	void make(not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);
	bool set(BitmapFont const& font, std::string_view text);
	bool set(Span<graphics::Glyph const> glyphs, glm::ivec2 atlas, std::string_view text);
	Span<Prop const> prop(BitmapFont const& font) const;
	Span<Prop const> prop(graphics::Texture const& atlas) const;
};
} // namespace le
