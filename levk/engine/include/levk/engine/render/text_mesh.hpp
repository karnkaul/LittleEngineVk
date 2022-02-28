#pragma once
#include <ktl/either.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le {
namespace graphics {
class Font;
}

struct TextLayout {
	glm::vec3 origin{};
	glm::vec2 pivot{};
	f32 scale = 1.0f;
	f32 lineSpacing = 1.5f;
};

class TextMesh {
  public:
	using RGBA = graphics::RGBA;
	using Font = graphics::Font;

	struct Obj {
		graphics::MeshPrimitive primitive;
		graphics::BPMaterialData bpMaterial;

		static Obj make(not_null<graphics::VRAM*> vram);
		graphics::DrawPrimitive drawPrimitive(Font& font, graphics::Geometry const& geometry, RGBA colour = colours::black);
		graphics::DrawPrimitive drawPrimitive(Font& font, std::string_view line, RGBA colour = colours::black, TextLayout const& layout = {});
	};

	struct Line {
		std::string line;
		TextLayout layout;
	};
	using Info = ktl::either<Line, graphics::Geometry>;

	TextMesh(not_null<graphics::VRAM*> vram) noexcept;
	TextMesh(not_null<Font*> font) noexcept;

	void font(not_null<Font*> font) noexcept { m_font = font; }
	Opt<Font> font() const noexcept { return m_font; }
	graphics::DrawPrimitive drawPrimitive() const;

	Info m_info;
	RGBA m_colour = colours::black;

  private:
	mutable Obj m_obj;
	Opt<Font> m_font{};
};
} // namespace le
