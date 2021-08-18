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

class Text2D {
  public:
	using Type = BitmapText::Type;

	Text2D() = default;
	Text2D(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram, Type type = Type::eDynamic);

	bool set(std::string_view text) { return m_font ? m_text.set(*m_font, text) : false; }
	Span<Prop const> props() const;
	graphics::TextFactory& factory() noexcept { return m_text.factory; }

  private:
	BitmapText m_text;
	BitmapFont const* m_font;
};
} // namespace le
