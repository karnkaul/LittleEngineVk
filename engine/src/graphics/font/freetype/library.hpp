#pragma once
#include <le/core/id.hpp>
#include <le/graphics/font/font_library.hpp>

#if !defined(LE_USE_FREETYPE)
#error "invalid project configuration"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

namespace le::graphics {
class FreetypeGlyphFactory : public GlyphSlot::Factory {
  public:
	FreetypeGlyphFactory(FreetypeGlyphFactory const&) = delete;
	FreetypeGlyphFactory(FreetypeGlyphFactory&&) = delete;
	auto operator=(FreetypeGlyphFactory const&) -> FreetypeGlyphFactory& = delete;
	auto operator=(FreetypeGlyphFactory&&) -> FreetypeGlyphFactory& = delete;

	FreetypeGlyphFactory(FT_Face face, std::vector<std::byte> bytes);
	~FreetypeGlyphFactory() override;

	auto set_height(TextHeight height) -> bool final;
	[[nodiscard]] auto height() const -> TextHeight final { return m_height; }
	[[nodiscard]] auto slot_for(Codepoint codepoint, std::vector<std::byte>& out_bytes) const -> GlyphSlot final;

  private:
	FT_Face m_face{};
	std::vector<std::byte> m_font_bytes{};
	TextHeight m_height{};
};

class Freetype : public FontLibrary {
  public:
	Freetype(Freetype const&) = delete;
	Freetype(Freetype&&) = delete;
	auto operator=(Freetype const&) -> Freetype& = delete;
	auto operator=(Freetype&&) -> Freetype& = delete;

	explicit Freetype();
	~Freetype() override;

	[[nodiscard]] auto load(std::vector<std::byte> bytes) const -> std::unique_ptr<GlyphSlot::Factory> final;

  private:
	FT_Library m_lib{};
};
} // namespace le::graphics
