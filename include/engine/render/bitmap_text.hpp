#pragma once
#include <engine/render/material.hpp>
#include <engine/scene/mesh_view.hpp>
#include <graphics/bitmap_glyph.hpp>
#include <graphics/mesh_primitive.hpp>
#include <optional>
#include <string>

namespace le {
namespace graphics {
class Texture;
class BitmapFont;
} // namespace graphics

using BitmapFont = graphics::BitmapFont;

struct TextGen {
	using Type = graphics::MeshPrimitive::Type;
	using Glyphs = graphics::BitmapGlyphMap;
	using Size = graphics::BitmapGlyph::Size;

	mutable Material material;
	glm::vec3 position{};
	glm::vec2 align{};
	graphics::RGBA colour;
	Size size = 1.0f;
	f32 nLinePad = 0.3f;

	MeshView mesh(graphics::MeshPrimitive const& primitive, graphics::Texture const& atlas) const noexcept;

	graphics::Geometry operator()(glm::uvec2 atlas, Glyphs const& glyphs, std::string_view text) const;
};

struct TextGenMesh {
	graphics::MeshPrimitive primitive;
	TextGen gen;
	graphics::BitmapFont const* font{};

	TextGenMesh(not_null<graphics::VRAM*> vram, graphics::BitmapFont const* font) : primitive(vram, graphics::MeshPrimitive::Type::eDynamic), font(font) {}

	MeshView mesh() const noexcept;
};

class BitmapText {
  public:
	using Type = TextGen::Type;

	BitmapText(not_null<BitmapFont const*> font, not_null<graphics::VRAM*> vram);

	virtual void set(std::string_view text, std::optional<f32> z = std::nullopt);
	MeshView mesh() const noexcept { return m_textMesh.mesh(); }
	TextGenMesh& textMesh() noexcept { return m_textMesh; }

  protected:
	TextGenMesh m_textMesh;
	not_null<BitmapFont const*> m_font;
};
} // namespace le
