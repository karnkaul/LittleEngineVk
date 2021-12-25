#pragma once
#include <graphics/font/atlas.hpp>
#include <graphics/utils/instant_command.hpp>

namespace le::graphics {
class Font {
  public:
	enum class Align { eMin, eCentre, eMax };

	using Size = FontFace::Size;
	class Pen;
	struct Info;
	struct PenInfo;

	Font(not_null<VRAM*> vram, Info info);

	bool load(Info info);

	FontFace const& face() const noexcept { return m_atlas.face(); }
	FontAtlas const& atlas() const noexcept { return m_atlas; }
	std::string_view name() const noexcept { return m_name; }

	static constexpr glm::vec2 pivot(Align horz = Align::eMin, Align vert = Align::eMin) noexcept;
	f32 scale(u32 height) const noexcept;

	bool write(Geometry& out, Glyph const& glyph, glm::vec3 origin = {}, f32 scale = 1.0f) const;

	not_null<VRAM*> m_vram;

  private:
	FontAtlas m_atlas;
	std::string m_name;
};

struct Font::PenInfo {
	glm::vec3 origin{};
	f32 scale = 1.0f;
	f32 lineSpacing = 1.0f;
	Geometry* out_geometry{};
};

class Font::Pen {
  public:
	static constexpr std::size_t npos = std::string::npos;

	Pen(not_null<Font*> font, PenInfo const& info = {});

	glm::vec2 lineExtent(std::string_view line) const;
	glm::vec2 textExtent(std::string_view text) const;

	Glyph const& glyph(Codepoint cp) const;
	void advance(Glyph const& glyph) noexcept { m_head += glm::vec3(glyph.advance, 0.0f) * m_info.scale; }
	std::string_view truncate(std::string_view line, f32 maxWidth) const;
	void align(std::string_view line, glm::vec2 pivot = {-0.5f, -0.5f});
	glm::vec3 writeLine(std::string_view line, glm::vec2 const* realign = {}, std::size_t const* retIdx = {}, f32 const* maxWidth = {});
	glm::vec3 writeText(std::string_view text, glm::vec2 const* realign = {});

	void lineFeed() noexcept;

	glm::vec3 const& head() const noexcept { return m_head; }
	void head(glm::vec3 pos) noexcept { m_head = pos; }
	f32 scale() const noexcept { return m_info.scale; }
	void scale(f32 s) noexcept;

  private:
	InstantCommand m_cmd;
	PenInfo m_info;
	glm::vec3 m_head;
	glm::vec3 m_begin;
	not_null<Font*> m_font;
};

struct Font::Info {
	FontAtlas::CreateInfo atlas;
	std::string name;
	Span<std::byte const> ttf;
	TPair<Codepoint> preload = {33U, 128U};
	Size size;
};

// impl

constexpr glm::vec2 Font::pivot(Align horz, Align vert) noexcept {
	auto const f = [](Align al) {
		switch (al) {
		default:
		case Align::eMin: return -0.5f;
		case Align::eCentre: return 0.0f;
		case Align::eMax: return 0.5f;
		}
	};
	return {f(horz), f(vert)};
}
} // namespace le::graphics
