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

class Font::Pen {
  public:
	Pen(not_null<Font*> font, Geometry* out = {}, glm::vec3 head = {}, f32 scale = 1.0f);

	glm::vec2 extent(std::string_view line) const;

	Glyph const& glyph(Codepoint cp) const;
	void advance(Glyph const& glyph) noexcept { m_head += glm::vec3(glyph.advance, 0.0f) * m_scale; }
	void align(std::string_view line, glm::vec2 pivot = {-0.5f, -0.5f});
	glm::vec3 const& write(std::string_view line, std::optional<glm::vec2> realign = std::nullopt);

	glm::vec3 const& head() const noexcept { return m_head; }
	void head(glm::vec3 pos) noexcept { m_head = pos; }
	f32 scale() const noexcept { return m_scale; }
	void scale(f32 s) noexcept;

  private:
	InstantCommand m_cmd;
	glm::vec3 m_head;
	f32 m_scale;
	Geometry* m_geom;
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
