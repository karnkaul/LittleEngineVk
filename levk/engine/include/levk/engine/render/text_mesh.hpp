#pragma once
#include <ktl/either.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/engine/scene/mesh_view.hpp>
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
		MeshPrimitive primitive;
		Material material;

		static Obj make(not_null<graphics::VRAM*> vram);
		MeshView mesh(Font& font, graphics::Geometry const& geometry, RGBA colour = colours::black);
		MeshView mesh(Font& font, std::string_view line, RGBA colour = colours::black, TextLayout const& layout = {});
	};

	struct Line {
		std::string line;
		TextLayout layout;
	};
	using Info = ktl::either<Line, graphics::Geometry>;

	TextMesh(not_null<Font*> font) noexcept;

	MeshView mesh() const;

	Info m_info;
	RGBA m_colour = colours::black;

  private:
	mutable Obj m_obj;
	not_null<Font*> m_font;
};
} // namespace le
