#include <spaced/graphics/fallback.hpp>

namespace spaced::graphics {
Fallback::Fallback() {
	auto pixel = std::array<std::uint8_t, 4>{};
	auto bitmap = Bitmap{pixel, {1, 1}};
	auto make_cubemap = [&] { return std::array{bitmap, bitmap, bitmap, bitmap, bitmap, bitmap}; };

	pixel = {0xff, 0xff, 0xff, 0xff};
	auto cubemap = make_cubemap();

	m_textures.white.write(bitmap);
	m_cubemaps.white.write(cubemap);

	pixel = {0x0, 0x0, 0x0, 0xff};
	cubemap = make_cubemap();

	m_textures.black.write(bitmap);
	m_cubemaps.black.write(cubemap);
}
} // namespace spaced::graphics
