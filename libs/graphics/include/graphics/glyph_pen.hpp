#pragma once
#include <core/not_null.hpp>
#include <graphics/geometry.hpp>
#include <graphics/glyph.hpp>
#include <graphics/render/rgba.hpp>

namespace le::graphics {
class GlyphPen {
  public:
	using Size = Glyph::Size;
	struct Scribe;

	struct Line {
		Geometry geometry;
		glm::vec2 extent{};
	};

	static constexpr std::size_t max_lines_v = 16;
	template <typename T>
	using lines_t = ktl::fixed_vector<T, max_lines_v>;
	struct Para {
		lines_t<Line> lines;
		f32 height{};
	};

	GlyphPen(not_null<GlyphMap const*> glyphs, glm::uvec2 atlas, Size size, glm::vec3 pos = {}, RGBA colour = colours::black) noexcept;

	bool generate(Geometry& out_geometry, Glyph const& glyph) const;

	///
	/// \brief Called before advance, before carriageReturn, and after writing all lines
	///
	virtual void advancePedal(std::size_t, Geometry*) {}

	Glyph const& write(u32 codepoint, Geometry* out = {}) const;
	glm::vec3 advance(u32 codepoint, Geometry* out = {});
	glm::vec3 carriageReturn() noexcept;
	glm::vec3 writeLine(std::string_view line, Geometry* out_geom = {}, std::size_t* out_index = {});

	static lines_t<std::string_view> splitLines(std::string_view paragraph) noexcept;
	Para writeLines(lines_t<std::string_view> const& lines, f32 pad, std::size_t* out_index = {});

	f32 size(Size size) noexcept;
	void colour(RGBA colour) noexcept { m_state.colour = colour; }
	void position(glm::vec3 pos, bool margin) noexcept;
	void position(f32 x, bool margin) noexcept { position({x, m_state.head.y, m_state.head.z}, margin); }
	f32 advanced() const noexcept { return m_state.advanced; }
	void reset(glm::vec3 pos) noexcept;

	Glyph const& glyph(u32 codepoint) const noexcept { return m_glyphs->glyph(codepoint); }
	glm::vec3 head() const noexcept { return m_state.head; }
	glm::vec2 extent() const noexcept { return m_state.lineExtent; }
	f32 scale() const noexcept { return m_state.scale; }

	f32 m_linePad = 5.0f;

  private:
	struct {
		glm::vec3 head{};
		glm::vec2 lineExtent{};
		f32 scale = 1.0f;
		f32 yBound{};
		f32 advanced{};
		f32 orgX{};
		RGBA colour = colours::black;
	} m_state;
	glm::uvec2 m_atlas{};
	not_null<GlyphMap const*> m_glyphs;
};

struct GlyphPen::Scribe {
	Geometry operator()(GlyphPen& out_pen, std::string_view paragraph, glm::vec2 align = {}, f32 pad = 5.0f) const;
};

// impl

inline f32 GlyphPen::size(Size size) noexcept {
	f32 scale{};
	u32 const y = m_glyphs->bounds().y;
	size.visit(ktl::overloaded{[&scale, y](u32 u) { scale = (f32)u / (f32)y; }, [&scale](f32 f) { scale = f; }});
	return m_state.scale = scale;
}

inline void GlyphPen::position(glm::vec3 pos, bool margin) noexcept {
	m_state.head = pos;
	if (margin) { m_state.orgX = pos.x; }
}

inline void GlyphPen::reset(glm::vec3 pos) noexcept {
	m_state = {};
	position(pos, true);
}
} // namespace le::graphics
