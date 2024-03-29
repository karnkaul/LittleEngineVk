#pragma once
#include <le/graphics/defer.hpp>
#include <le/graphics/image_view.hpp>
#include <le/graphics/resource.hpp>
#include <le/graphics/texture_sampler.hpp>

namespace le::graphics {
enum class ColourSpace : std::uint8_t { eSrgb, eLinear };

class Texture {
  public:
	using Sampler = TextureSampler;

	explicit Texture(ColourSpace colour_space = ColourSpace::eSrgb, bool mip_map = true);

	auto write(Bitmap const& bitmap) -> bool;

	auto set_image(std::unique_ptr<Image> image) -> bool;

	[[nodiscard]] auto image() const -> Image const& { return *m_image.get(); }
	[[nodiscard]] auto view() const -> ImageView;
	[[nodiscard]] auto is_mip_mapped() const -> bool { return m_image.get()->mip_levels() > 1; }
	[[nodiscard]] auto colour_space() const -> ColourSpace;

	Sampler sampler{};

  protected:
	Texture(ImageCreateInfo const& create_info);

	Defer<std::unique_ptr<Image>> m_image{};

	friend class DynamicAtlas;
};

class Cubemap : public Texture {
  public:
	explicit Cubemap(ColourSpace colour_space = ColourSpace::eSrgb);

	auto write(std::span<Bitmap const, Image::cubemap_layers_v> bitmaps) -> bool;
};
} // namespace le::graphics
