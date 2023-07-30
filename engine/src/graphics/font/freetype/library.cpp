#include <graphics/font/freetype/library.hpp>
#include <spaced/core/logger.hpp>
#include <spaced/resources/bin_data.hpp>

namespace spaced::graphics {
FreetypeGlyphFactory::FreetypeGlyphFactory(FT_Face face, std::vector<std::uint8_t> bytes) : m_face(face), m_font_bytes(std::move(bytes)) {}

FreetypeGlyphFactory::~FreetypeGlyphFactory() { FT_Done_Face(m_face); }

auto FreetypeGlyphFactory::set_height(TextHeight height) -> bool {
	if (FT_Set_Pixel_Sizes(m_face, 0, static_cast<FT_UInt>(height)) == FT_Err_Ok) {
		m_height = height;
		return true;
	}
	return false;
}

auto FreetypeGlyphFactory::slot_for(Codepoint codepoint, std::vector<std::uint8_t>& out_bytes) const -> GlyphSlot {
	if (m_face == nullptr) { return {}; }
	if (FT_Load_Char(m_face, static_cast<FT_ULong>(codepoint), FT_LOAD_RENDER) != FT_Err_Ok) { return {}; }
	if (m_face->glyph == nullptr) { return {}; }
	auto ret = GlyphSlot{.codepoint = codepoint};
	auto const& glyph = *m_face->glyph;
	ret.pixmap.extent = {glyph.bitmap.width, glyph.bitmap.rows};
	auto const size = static_cast<std::size_t>(ret.pixmap.extent.x * ret.pixmap.extent.y * GlyphSlot::Pixmap::channels_v);
	if (size > 0) { ret.pixmap.bytes = resize_and_overwrite(out_bytes, {glyph.bitmap.buffer, size}); }
	ret.left_top = {glyph.bitmap_left, glyph.bitmap_top};
	ret.advance = {static_cast<std::int32_t>(m_face->glyph->advance.x), static_cast<std::int32_t>(m_face->glyph->advance.y)};
	return ret;
}

Freetype::Freetype() {
	if (FT_Init_FreeType(&m_lib) != FT_Err_Ok) { logger::Logger{"Freetype"}.error("failed to initialize freetype"); }
}

Freetype::~Freetype() { FT_Done_FreeType(m_lib); }

auto Freetype::load(std::vector<std::uint8_t> bytes) const -> std::unique_ptr<GlyphSlot::Factory> {
	if (m_lib == nullptr) { return {}; }
	auto* face = FT_Face{};
	// NOLINTNEXTLINE
	if (FT_New_Memory_Face(m_lib, reinterpret_cast<FT_Byte const*>(bytes.data()), static_cast<FT_Long>(bytes.size()), 0, &face) != FT_Err_Ok) { return {}; }
	return std::make_unique<FreetypeGlyphFactory>(face, std::move(bytes));
}
} // namespace spaced::graphics
