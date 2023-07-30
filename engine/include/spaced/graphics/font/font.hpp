#pragma once
#include <spaced/graphics/font/font_atlas.hpp>
#include <spaced/graphics/geometry.hpp>
#include <spaced/graphics/rgba.hpp>
#include <optional>

namespace spaced::graphics {
class Font {
  public:
	class Pen;

	static auto try_make(std::vector<std::uint8_t> file_bytes) -> std::optional<Font>;

	explicit Font(NotNull<std::unique_ptr<GlyphSlot::Factory>> slot_factory);

	[[nodiscard]] auto get_font_atlas(TextHeight height) -> FontAtlas&;

	[[nodiscard]] auto glyph_for(TextHeight height, Codepoint codepoint) -> Glyph { return get_font_atlas(height).glyph_for(codepoint); }

  private:
	std::unordered_map<TextHeight, FontAtlas> m_atlases{};
	NotNull<std::unique_ptr<GlyphSlot::Factory>> m_slot_factory;
};

class Font::Pen {
  public:
	Pen(Font& font, TextHeight height = TextHeight::eDefault, float scale = 1.0f) : m_font(font), m_height(clamp(height)), m_scale(scale) {}

	auto advance(std::string_view line) -> Pen&;
	auto generate_quads(std::string_view line) -> Geometry;
	auto generate_quads(Geometry& out, std::string_view line) -> Pen&;

	[[nodiscard]] auto calc_line_extent(std::string_view line) const -> glm::vec2;

	[[nodiscard]] auto get_texture() const -> Texture const& { return m_font.get_font_atlas(m_height).get_texture(); }
	[[nodiscard]] auto get_height() const -> TextHeight { return m_height; }

	glm::vec3 cursor{};
	Rgba vertex_colour{white_v};

  private:
	struct Writer;

	// NOLINTNEXTLINE
	Font& m_font;
	TextHeight m_height{};
	float m_scale{};
};
} // namespace spaced::graphics
