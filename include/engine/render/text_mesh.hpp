#pragma once
#include <engine/render/material.hpp>
#include <engine/scene/mesh_view.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le {
namespace graphics {
class Font;
}

class TextMesh {
  public:
	using RGBA = graphics::RGBA;
	using Font = graphics::Font;

	struct Info {
		glm::vec3 origin{};
		glm::vec2 pivot{};
		f32 scale = 1.0f;
		RGBA colour = colours::black;
	};

	struct Obj {
		MeshPrimitive primitive;
		Material material;

		static Obj make(not_null<graphics::VRAM*> vram);
		MeshView mesh(Font& font, std::string_view line, Info const& info);
	};

	TextMesh(not_null<Font*> font) noexcept;

	MeshView mesh() const;

	std::string m_line;
	Info m_info;

  private:
	mutable Obj m_obj;
	not_null<Font*> m_font;
};
} // namespace le
