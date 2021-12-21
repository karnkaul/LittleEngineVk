#include <core/log_channel.hpp>
#include <ft/ft.hpp>

namespace le::graphics {
FTLib FTLib::make() noexcept {
	FTLib ret;
	if (auto err = FT_Init_FreeType(&ret.lib)) {
		logE(LC_LibUser, "[Graphics] Failed to initialize freetype!");
		return {};
	}
	return ret;
}

FTFace FTFace::make(FTLib const& lib, Span<std::byte const> bytes) noexcept {
	static_assert(sizeof(FT_Byte) == sizeof(std::byte));
	FTFace ret;
	if (auto err = FT_New_Memory_Face(lib.lib, reinterpret_cast<FT_Byte const*>(bytes.data()), static_cast<FT_Long>(bytes.size()), 0, &ret.face)) {
		logE(LC_LibUser, "[Graphics] Failed to make font face");
		return {};
	}
	return ret;
}

FTFace FTFace::make(FTLib const& lib, char const* path) noexcept {
	FTFace ret;
	if (auto err = FT_New_Face(lib.lib, path, 0, &ret.face)) {
		logE(LC_LibUser, "[Graphics] Failed to make font face");
		return {};
	}
	return ret;
}

bool FTFace::setCharSize(glm::uvec2 const size, glm::uvec2 const res) const noexcept {
	if (auto err = FT_Set_Char_Size(face, size.x, size.y, res.x, res.y)) {
		logW("[Graphics] Failed to set font face char size");
		return false;
	}
	return true;
}

bool FTFace::setPixelSize(glm::uvec2 const size) const noexcept {
	if (auto err = FT_Set_Pixel_Sizes(face, size.x, size.y)) {
		logW("[Graphics] Failed to set font face pixel size");
		return false;
	}
	return true;
}

bool FTFace::loadGlyph(u32 codepoint, FT_Render_Mode mode) const noexcept {
	if (auto err = FT_Load_Glyph(face, FT_Get_Char_Index(face, codepoint), FT_LOAD_DEFAULT)) {
		logW("[Graphics] Failed to load glyph for codepoint [{} ({})]", static_cast<unsigned char>(codepoint), codepoint);
		return false;
	}
	if (auto err = FT_Render_Glyph(face->glyph, mode)) {
		logW("[Graphics] Failed to render glyph for codepoint [{} ({})]", static_cast<unsigned char>(codepoint), codepoint);
		return false;
	}
	return true;
}

std::vector<u8> FTFace::buildGlyphImage() const {
	std::vector<u8> ret;
	if (face && face->glyph && face->glyph->bitmap.width > 0U && face->glyph->bitmap.rows > 0U) {
		ret.reserve(face->glyph->bitmap.width & face->glyph->bitmap.rows * 4U);
		u8 const* line = face->glyph->bitmap.buffer;
		for (u32 row = 0; row < face->glyph->bitmap.rows; ++row) {
			u8 const* src = line;
			for (u32 col = 0; col < face->glyph->bitmap.width; ++col) {
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

void FTDeleter::operator()(FTLib const& lib) const noexcept { FT_Done_FreeType(lib.lib); }
void FTDeleter::operator()(FTFace const& face) const noexcept { FT_Done_Face(face.face); }

void foo() { FTUnique<FTLib> ftlib; }
} // namespace le::graphics
