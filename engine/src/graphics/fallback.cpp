#include <le/graphics/fallback.hpp>

namespace le::graphics {
Fallback::Fallback() {
	auto pixel = std::array<std::byte, 4>{};
	auto bitmap = Bitmap{pixel, {1, 1}};
	auto make_cubemap = [&] { return std::array{bitmap, bitmap, bitmap, bitmap, bitmap, bitmap}; };

	pixel = {std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}};
	auto cubemap = make_cubemap();

	m_textures.white.write(bitmap);
	m_cubemaps.white.write(cubemap);

	pixel = {std::byte{}, std::byte{}, std::byte{}, std::byte{0xff}};
	cubemap = make_cubemap();

	m_textures.black.write(bitmap);
	m_cubemaps.black.write(cubemap);
}
} // namespace le::graphics
