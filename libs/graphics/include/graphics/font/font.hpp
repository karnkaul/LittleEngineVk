#pragma once
#include <graphics/font/atlas.hpp>
#include <graphics/utils/instant_command.hpp>
#include <vector>

namespace le::graphics {
class Font {
  public:
	static constexpr f32 default_spacing_v = 1.5f;

	enum class Align { eMin, eCentre, eMax };

	using Height = FontFace::Height;
	class Pen;
	struct PenInfo;

	struct Info {
		FontAtlas::CreateInfo atlas;
		std::string name;
		Span<std::byte const> ttf;
		TPair<Codepoint> preload = {33U, 128U};
		Height height;
	};

	Font(not_null<VRAM*> vram, Info info);

	bool add(Height height);
	bool contains(Height height) const noexcept { return m_atlases.contains(height); }
	bool remove(Height height);
	std::size_t sizeCount() const noexcept { return m_atlases.size(); }
	std::vector<Height> sizes() const;

	std::string_view name() const noexcept { return m_info.name; }
	FontFace const& face(Opt<Height const> size = {}) const noexcept { return atlas(size).face(); }
	FontAtlas const& atlas(Opt<Height const> size = {}) const noexcept;

	static constexpr glm::vec2 pivot(Align horz = Align::eMin, Align vert = Align::eMin) noexcept;
	f32 scale(u32 height, Opt<Height const> size = {}) const noexcept;

	bool write(Geometry& out, Glyph const& glyph, glm::vec3 origin = {}, f32 scale = 1.0f) const;

	not_null<VRAM*> m_vram;

  private:
	bool load(FontAtlas& out, Height size);

	std::unordered_map<u32, FontAtlas> m_atlases;
	Info m_info;
};

struct Font::PenInfo {
	glm::vec3 origin{};
	f32 scale = 1.0f;
	f32 lineSpacing = default_spacing_v;
	Opt<Geometry> out_geometry{};
	Opt<Height const> customSize{};
};

class Font::Pen {
  public:
	static constexpr std::size_t npos = std::string::npos;

	Pen(not_null<Font*> font, PenInfo const& info = {});

	glm::vec2 lineExtent(std::string_view line) const;
	glm::vec2 textExtent(std::string_view text) const;

	Glyph glyph(Codepoint cp) const;
	void advance(Glyph const& glyph) noexcept { m_head += glm::vec3(glyph.advance, 0.0f) * m_info.scale; }
	std::string_view truncate(std::string_view line, f32 maxWidth) const;
	void align(std::string_view line, glm::vec2 pivot = {-0.5f, -0.5f});
	glm::vec3 writeLine(std::string_view line, Opt<glm::vec2 const> realign = {}, Opt<std::size_t const> retIdx = {}, Opt<f32 const> maxWidth = {});
	glm::vec3 writeText(std::string_view text, Opt<glm::vec2 const> realign = {});

	void lineFeed() noexcept;

	glm::vec3 const& head() const noexcept { return m_head; }
	void head(glm::vec3 pos) noexcept { m_head = pos; }
	f32 scale() const noexcept { return m_info.scale; }
	void scale(f32 s) noexcept;

  private:
	FontAtlas& atlas() const;

	InstantCommand m_cmd;
	PenInfo m_info;
	glm::vec3 m_head;
	glm::vec3 m_begin;
	not_null<Font*> m_font;
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
