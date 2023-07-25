#include <spaced/engine/core/zip_ranges.hpp>
#include <spaced/engine/graphics/texture.hpp>
#include <algorithm>
#include <array>

namespace spaced::graphics {
namespace {
constexpr auto to_format(ColourSpace const colour_space) -> vk::Format {
	if (colour_space == ColourSpace::eLinear) { return vk::Format::eR8G8B8A8Unorm; }
	return vk::Format::eR8G8B8A8Srgb;
}
} // namespace

Texture::Texture(ColourSpace const colour_space) : Texture(ImageCreateInfo{.format = to_format(colour_space)}) {}

Texture::Texture(ImageCreateInfo const& create_info) : m_image(std::make_unique<Image>(create_info)) {}

auto Texture::view() const -> ImageView {
	return ImageView{m_image.get()->image(), m_image.get()->image_view(), m_image.get()->extent(), m_image.get()->format()};
}

auto Texture::colour_space() const -> ColourSpace { return m_image.get()->format() == vk::Format::eR8G8B8A8Srgb ? ColourSpace::eSrgb : ColourSpace::eLinear; }

auto Texture::write(Bitmap const& bitmap) -> bool {
	auto const layer = std::array<Image::Layer, 1>{bitmap.bytes};
	return m_image.get()->copy_from(layer, {bitmap.extent.x, bitmap.extent.y}, {});
}

Cubemap::Cubemap(ColourSpace colour_space) : Texture(ImageCreateInfo{.format = to_format(colour_space), .view_type = vk::ImageViewType::eCube}) {}

auto Cubemap::write(std::span<Bitmap const, Image::cubemap_layers_v> bitmaps) -> bool {
	auto const target_extent = bitmaps[0].extent;
	if (!std::ranges::all_of(bitmaps, [target_extent](Bitmap const& bitmap) { return bitmap.extent == target_extent; })) { return false; }
	auto const layers = [&] {
		auto ret = std::array<Image::Layer, Image::cubemap_layers_v>{};
		for (auto [in, out] : zip_ranges(bitmaps, ret)) { out = in.bytes; }
		return ret;
	}();
	return m_image.get()->copy_from(layers, {target_extent.x, target_extent.y}, {});
}
} // namespace spaced::graphics
