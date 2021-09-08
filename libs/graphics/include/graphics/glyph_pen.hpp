#pragma once
#include <core/not_null.hpp>
#include <graphics/geometry.hpp>
#include <graphics/glyph.hpp>
#include <graphics/render/rgba.hpp>

namespace le::graphics {
///
/// \brief Writes glyphs geometry, maintains write-head position; can write and align multiple lines
///
class GlyphPen {
  public:
	using Size = Glyph::Size;

	static constexpr std::size_t max_lines_v = 16;
	template <typename T>
	using lines_t = ktl::fixed_vector<T, max_lines_v>;

	///
	/// \brief Geometry for a line of text, and its size
	///
	struct Line {
		Geometry geometry;
		glm::vec2 extent{};
	};
	///
	/// \brief Indices into a line and character within it
	///
	struct LineIndex {
		std::size_t line{};
		std::size_t character{};
	};
	///
	/// \brief Metadata for writing multiple lines
	///
	struct LineInfo {
		///
		/// \brief Proportion of line height to add as padding between lines
		///
		f32 nLinePad{};
		///
		/// \brief Whether to produce geometry
		///
		bool produceGeometry = true;
		///
		/// \brief Index of character to return write-head position for
		///
		std::size_t const* returnIndex{};
	};
	///
	/// \brief Paragraph ready to be aligned, with write-head position
	///
	struct Para {
		lines_t<Line> lines;
		glm::vec3 head{};
	};

	GlyphPen(not_null<GlyphMap const*> glyphs, glm::uvec2 atlas, Size size, glm::vec3 pos = {}, RGBA colour = colours::black) noexcept;

	///
	/// \brief Append quad to geometry if valid glyph
	/// \returns true if valud glyph
	///
	bool generate(Geometry& out_geometry, Glyph const& glyph) const;
	///
	/// \brief Obtain glyph for codepoint, generate into out if not null
	///
	Glyph const& write(u32 codepoint, Geometry* out = {}) const;
	///
	/// \brief Write glyph for codepoint and advance write head
	/// \returns position of write head at end
	///
	glm::vec3 advance(u32 codepoint, Geometry* out = {});
	///
	/// \brief Return write head to start of line
	/// \returns position of write head at end
	///
	glm::vec3 carriageReturn() noexcept;
	///
	/// \brief Move write head to next line
	/// \returns position of write head at end
	///
	glm::vec3 lineFeed(f32 nPad) noexcept;
	///
	/// \brief Write a line of text (without line feed)
	/// \returns position of write head at end, or of head when writing char at index, if not null
	///
	glm::vec3 writeLine(std::string_view line, Geometry* out_geom = {}, std::size_t const* index = {});
	///
	/// \brief Offset write head +Y for lineCount lines
	///
	glm::vec3 offsetY(std::size_t lineCount, f32 nPad) noexcept;

	///
	/// \brief Split paragraph into lines
	///
	static lines_t<std::string_view> splitLines(std::string_view paragraph) noexcept;
	///
	/// \brief Convert string index to line index
	///
	static LineIndex lineIndex(lines_t<std::string_view> const& lines, std::size_t index) noexcept;
	///
	/// \brief Offset line according to alignment
	///
	glm::vec3 alignOffset(glm::vec2 lineExtent, glm::vec2 nAlign) const noexcept;
	///
	/// \brief Write multiple lines
	///
	Para writeLines(lines_t<std::string_view> const& lines, LineInfo const& info);
	///
	/// \brief Align all lines, append to out if not null
	///
	void alignLines(lines_t<Line>& out_lines, glm::vec2 nAlign, Geometry* out = {}) const;
	///
	/// \brief Generate geometry for paragraph
	///
	Geometry generate(std::string_view paragraph, f32 nLinePad, glm::vec2 nAlign = {});

	f32 size(Size size) noexcept;
	void colour(RGBA colour) noexcept { m_colour = colour; }
	void position(glm::vec3 pos, bool margin) noexcept;
	void position(f32 x, bool margin) noexcept { position({x, m_state.head.y, m_state.head.z}, margin); }
	f32 advanced() const noexcept { return m_state.advanced; }
	void reset(glm::vec3 pos) noexcept;

	GlyphMap const& glyphs() const noexcept { return *m_glyphs; }
	Glyph const& glyph(u32 codepoint) const noexcept { return m_glyphs->glyph(codepoint); }
	glm::vec3 head() const noexcept { return m_state.head; }
	glm::vec2 lineExtent() const noexcept { return m_state.lineExtent; }
	glm::vec2 extent() const noexcept { return m_state.totalExtent; }
	f32 scale() const noexcept { return m_scale; }
	f32 lineHeight() const noexcept { return m_glyphs->lineHeight(m_scale); }
	std::size_t lineCount() const noexcept { return m_state.lineCount; }

  private:
	struct {
		glm::vec3 head{};
		glm::vec2 lineExtent{};
		glm::vec2 totalExtent{};
		f32 advanced{};
		f32 orgX{};
		std::size_t lineCount{};
	} m_state;
	glm::uvec2 m_atlas{};
	RGBA m_colour = colours::black;
	f32 m_scale = 1.0f;
	not_null<GlyphMap const*> m_glyphs;
};

// impl

inline f32 GlyphPen::size(Size size) noexcept {
	f32 scale{};
	u32 const y = m_glyphs->bounds().y;
	size.visit(ktl::overloaded{[&scale, y](u32 u) { scale = (f32)u / (f32)y; }, [&scale](f32 f) { scale = f; }});
	return m_scale = scale;
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
