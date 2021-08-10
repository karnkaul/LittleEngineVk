#pragma once
#include <optional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/rgba.hpp>
#include <ktl/either.hpp>

namespace le::graphics {
struct Glyph {
	u8 ch = '\0';
	glm::ivec2 st = glm::ivec2(0);
	glm::ivec2 uv = glm::ivec2(0);
	glm::ivec2 cell = glm::ivec2(0);
	glm::ivec2 offset = glm::ivec2(0);
	s32 xAdv = 0;
	bool blank = false;
};

struct TextFactory {
	using Size = ktl::either<u32, f32>;

	struct Layout {
		glm::ivec2 maxBounds = {};
		u32 lineCount = 0;
		f32 lineHeight = 0.0f;
		f32 textHeight = 0.0f;
		f32 scale = 1.0f;
		f32 linePad = 0.2f;
	};

	glm::vec3 pos = glm::vec3(0.0f);
	glm::vec2 align = {};
	Size size = 1.0f;
	f32 nYPad = 0.2f;
	RGBA colour = colours::white;

	Geometry generate(Span<Glyph const> glyphs, std::string_view text, glm::ivec2 texSize, std::optional<Layout> layout = std::nullopt) const noexcept;
	glm::ivec2 glyphBounds(Span<Glyph const> glyphs, std::string_view text = {}) const noexcept;
	Layout layout(Span<Glyph const> glyphs, std::string_view text, Size size = 1.0f, f32 nPadY = 0.1f) const noexcept;
};
} // namespace le::graphics
