#pragma once
#include <spaced/graphics/bitmap.hpp>
#include <spaced/graphics/defer.hpp>
#include <spaced/graphics/image_view.hpp>
#include <spaced/graphics/resource.hpp>
#include <spaced/graphics/texture_sampler.hpp>

namespace spaced::graphics {
enum class ColourSpace : std::uint8_t { eSrgb, eLinear };

class Texture {
  public:
	using Sampler = TextureSampler;

	explicit Texture(ColourSpace colour_space = ColourSpace::eSrgb);

	auto write(Bitmap const& bitmap) -> bool;

	[[nodiscard]] auto image() const -> Image const& { return *m_image.get(); }
	[[nodiscard]] auto view() const -> ImageView;
	[[nodiscard]] auto is_mip_mapped() const -> bool { return m_image.get()->mip_levels() > 1; }
	[[nodiscard]] auto colour_space() const -> ColourSpace;

	Sampler sampler{};

  protected:
	Texture(ImageCreateInfo const& create_info);

	Defer<std::unique_ptr<Image>> m_image{};
};

class Cubemap : public Texture {
  public:
	explicit Cubemap(ColourSpace colour_space = ColourSpace::eSrgb);

	auto write(std::span<Bitmap const, Image::cubemap_layers_v> bitmaps) -> bool;
};
} // namespace spaced::graphics
