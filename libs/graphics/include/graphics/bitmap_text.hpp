#pragma once
#include <variant>
#include <core/colour.hpp>
#include <core/maths.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <graphics/geometry.hpp>

namespace le::graphics {
struct Glyph {
	u8 ch = '\0';
	glm::ivec2 st = glm::ivec2(0);
	glm::ivec2 uv = glm::ivec2(0);
	glm::ivec2 cell = glm::ivec2(0);
	glm::ivec2 offset = glm::ivec2(0);
	s32 xAdv = 0;
	bool bBlank = false;
};

struct BitmapText {
	enum class HAlign : s8 { Centre = 0, Left, Right };
	enum class VAlign : s8 { Middle = 0, Top, Bottom };

	using Size = std::variant<u32, f32>;

	struct Layout {
		glm::ivec2 maxBounds = {};
		u32 lineCount = 0;
		f32 lineHeight = 0.0f;
		f32 textHeight = 0.0f;
		f32 scale = 1.0f;
		f32 linePad = 0.2f;
	};

	std::string text;
	glm::vec3 pos = glm::vec3(0.0f);
	Size size = 1.0f;
	f32 nYPad = 0.2f;
	HAlign halign = HAlign::Centre;
	VAlign valign = VAlign::Middle;
	Colour colour = colours::white;

	Geometry generate(View<Glyph> glyphs, glm::ivec2 texSize, std::optional<Layout> layout = std::nullopt) const noexcept;
	glm::ivec2 glyphBounds(View<Glyph> glyphs, std::string_view text = {}) const noexcept;
	Layout layout(View<Glyph> glyphs, std::string_view text, Size size = 1.0f, f32 nPadY = 0.1f) const noexcept;
};
} // namespace le::graphics
