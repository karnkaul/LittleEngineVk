#include <le/graphics/font/font_library.hpp>

#if defined(LE_USE_FREETYPE)
#include <graphics/font/freetype/library.hpp>
#endif

namespace le::graphics {
auto FontLibrary::make() -> std::unique_ptr<FontLibrary> {
	auto ret = std::unique_ptr<FontLibrary>{};
#if defined(LE_USE_FREETYPE)
	ret = std::make_unique<Freetype>();
#else
	ret = std::make_unique<FontLibrary::Null>();
#endif
	return ret;
}
} // namespace le::graphics
