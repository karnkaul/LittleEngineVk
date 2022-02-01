#include <ft/ft.hpp>
#include <levk/core/log_channel.hpp>

namespace le::graphics {
FTLib FTLib::make() noexcept {
	FTLib ret;
	if (FT_Init_FreeType(&ret.lib)) {
		logE(LC_LibUser, "[Graphics] Failed to initialize freetype!");
		return {};
	}
	return ret;
}

FTFace FTFace::make(FTLib const& lib, Span<std::byte const> bytes) noexcept {
	static_assert(sizeof(FT_Byte) == sizeof(std::byte));
	FTFace ret;
	if (FT_New_Memory_Face(lib.lib, reinterpret_cast<FT_Byte const*>(bytes.data()), static_cast<FT_Long>(bytes.size()), 0, &ret.face)) {
		logE(LC_LibUser, "[Graphics] Failed to make font face");
		return {};
	}
	return ret;
}

FTFace FTFace::make(FTLib const& lib, char const* path) noexcept {
	FTFace ret;
	if (FT_New_Face(lib.lib, path, 0, &ret.face)) {
		logE(LC_LibUser, "[Graphics] Failed to make font face");
		return {};
	}
	return ret;
}

bool FTFace::setCharSize(glm::uvec2 const size, glm::uvec2 const res) const noexcept {
	if (FT_Set_Char_Size(face, size.x, size.y, res.x, res.y)) {
		logW("[Graphics] Failed to set font face char size");
		return false;
	}
	return true;
}

bool FTFace::setPixelSize(glm::uvec2 const size) const noexcept {
	if (FT_Set_Pixel_Sizes(face, size.x, size.y)) {
		logW("[Graphics] Failed to set font face pixel size");
		return false;
	}
	return true;
}

FTFace::ID FTFace::glyphIndex(u32 codepoint) const noexcept { return FT_Get_Char_Index(face, codepoint); }

bool FTFace::loadGlyph(ID index, FT_Render_Mode mode) const noexcept {
	if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
		logW("[Graphics] Failed to load glyph for index [{}]", index);
		return false;
	}
	if (FT_Render_Glyph(face->glyph, mode)) {
		logW("[Graphics] Failed to render glyph for index [{}]", index);
		return false;
	}
	return true;
}

std::vector<u8> FTFace::buildGlyphImage() const {
	std::vector<u8> ret;
	if (face && face->glyph && face->glyph->bitmap.width > 0U && face->glyph->bitmap.rows > 0U) {
		Extent2D const extent{face->glyph->bitmap.width, face->glyph->bitmap.rows};
		ret.reserve(extent.x & extent.y * 4U);
		u8 const* line = face->glyph->bitmap.buffer;
		for (u32 row = 0; row < extent.y; ++row) {
			u8 const* src = line;
			for (u32 col = 0; col < extent.x; ++col) {
				ret.push_back(0xff);
				ret.push_back(0xff);
				ret.push_back(0xff);
				ret.push_back(*src++);
			}
			line += face->glyph->bitmap.pitch;
		}
	}
	return ret;
}

Extent2D FTFace::glyphExtent() const {
	if (face && face->glyph) { return {face->glyph->bitmap.width, face->glyph->bitmap.rows}; }
	return {};
}

void FTDeleter::operator()(FTLib const& lib) const noexcept { FT_Done_FreeType(lib.lib); }
void FTDeleter::operator()(FTFace const& face) const noexcept { FT_Done_Face(face.face); }

void foo() { FTUnique<FTLib> ftlib; }
} // namespace le::graphics
