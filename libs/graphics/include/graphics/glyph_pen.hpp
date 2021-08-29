#pragma once
#include <core/not_null.hpp>
#include <graphics/bitmap_glyph.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/rgba.hpp>
#include <ktl/either.hpp>

namespace le::graphics {
class BitmapGlyphPen {
  public:
	using Size = ktl::either<u32, f32>;

	BitmapGlyphPen(not_null<BitmapGlyphArray const*> glyphs, glm::uvec2 atlas, Size size, glm::vec3 pos = {}, RGBA colour = colours::black) noexcept;

	bool generate(Geometry& out_geometry, BitmapGlyph const& glyph) const;

	glm::vec3 advance(u8 ch, Geometry* out = {});
	glm::vec3 carriageReturn(bool nextLine = true) noexcept;
	glm::vec3 writeLine(std::string_view line, Geometry* out = {});
	glm::vec3 write(std::string_view text, glm::vec2 align = {}, Geometry* out = {});
	Geometry generate(std::string_view text, glm::vec2 align = {});

	f32 resize(Size size) noexcept;
	void reset(glm::vec3 pos) noexcept;
	void reset(RGBA colour) noexcept { m_state.colour = colour; }

	BitmapGlyph const& glyph(u8 ch) const noexcept { return m_glyphs->glyph(ch); }
	glm::vec3 head() const noexcept { return m_state.head; }
	f32 scale() const noexcept { return m_state.scale; }
	u32 lineCount() const noexcept { return m_state.lineCount; }

	f32 m_linePad{};

  private:
	struct {
		glm::vec3 head{};
		glm::vec2 lineExtent{};
		f32 scale = 1.0f;
		f32 orgX{};
		f32 orgZ{};
		f32 firstChar{};
		u32 lineCount{};
		RGBA colour = colours::black;
	} m_state;
	glm::uvec2 m_atlas{};
	not_null<BitmapGlyphArray const*> m_glyphs;
};

// impl

inline f32 BitmapGlyphPen::resize(Size size) noexcept {
	f32 scale{};
	u32 const y = m_glyphs->bounds().y;
	size.visit(ktl::overloaded{[&scale, y](u32 u) { scale = (f32)u / (f32)y; }, [&scale](f32 f) { scale = f; }});
	return m_state.scale = scale;
}

inline void BitmapGlyphPen::reset(glm::vec3 pos) noexcept {
	m_state = {};
	m_state.head = pos;
	m_state.orgX = pos.x;
	m_state.orgZ = pos.z;
}
} // namespace le::graphics
