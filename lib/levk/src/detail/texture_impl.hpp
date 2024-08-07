#pragma once
#include <levk/core/is_positive.hpp>
#include <levk/image_file.hpp>
#include <levk/render_device.hpp>
#include <levk/texture.hpp>
#include <levk/util.hpp>

namespace levk {
class TextureImpl {
  public:
	using Flag = TextureFlag;
	using Flags = TextureFlags;

	TextureImpl(NotNull<IRenderDevice*> render_device, BitmapView const bitmap, Flags const flags) : m_device(render_device) {
		auto ici = ImageCreateInfo{
			.extent = to_vk_extent(bitmap.extent),
			.mip_map = !flags.test(Flag::eNoMipMaps),
		};
		if (flags.test(Flag::eLinear)) { ici.format = vk::Format::eR8G8B8A8Unorm; }
		m_image = render_device->create_image(ici);
		do_recreate(bitmap);
	}

	TextureImpl(NotNull<IRenderDevice*> render_device, CubemapLayers const& layers, Flags const flags) : m_device(render_device) {
		auto ici = ImageCreateInfo{
			.extent = to_vk_extent(layers.front().extent),
			.view_type = vk::ImageViewType::eCube,
			.mip_map = !flags.test(Flag::eNoMipMaps),
		};
		if (flags.test(Flag::eLinear)) { ici.format = vk::Format::eR8G8B8A8Unorm; }
		m_image = render_device->create_image(ici);
		m_image->write_cube(layers);
	}

	[[nodiscard]] auto do_get_size() const -> glm::ivec2 {
		if (!m_image) { return {}; }
		return to_glm_vec2(m_image->get_image_info().extent);
	}

	[[nodiscard]] auto do_get_descriptor_info(TextureSampler const& sampler, std::uint32_t const binding) const -> DescriptorInfo {
		if (!m_image) { return {}; }

		return DescriptorInfo{
			.payload = vk::DescriptorImageInfo{m_device->get_sampler(sampler), m_image->get_image_info().view, m_image->get_image_info().layout},
			.type = vk::DescriptorType::eCombinedImageSampler,
			.binding = binding,
		};
	}

	void do_recreate(BitmapView const bitmap) {
		if (!m_image) { return; }
		m_image->resize(to_vk_extent(bitmap.extent));
		m_image->overwrite(bitmap);
	}

  private:
	NotNull<IRenderDevice*> m_device;
	std::unique_ptr<IRenderImage> m_image{};
};

class Texture : public ITexture, private TextureImpl {
  public:
	Texture(NotNull<IRenderDevice*> render_device, BitmapView const bitmap, Flags const flags) : TextureImpl(render_device, bitmap, flags) {}

  private:
	[[nodiscard]] auto get_size() const -> glm::ivec2 final { return do_get_size(); }
	[[nodiscard]] auto get_descriptor_info(std::uint32_t binding) const -> DescriptorInfo final { return do_get_descriptor_info(sampler, binding); }
};

class DynamicTexture : public IDynamicTexture, private TextureImpl {
  public:
	DynamicTexture(NotNull<IRenderDevice*> render_device, BitmapView const bitmap, Flags const flags) : TextureImpl(render_device, bitmap, flags) {}

  private:
	[[nodiscard]] auto get_size() const -> glm::ivec2 final { return do_get_size(); }
	[[nodiscard]] auto get_descriptor_info(std::uint32_t binding) const -> DescriptorInfo final { return do_get_descriptor_info(sampler, binding); }

	void recreate(BitmapView bitmap) final { do_recreate(bitmap); }
};

class Cubemap : public ICubemap, private TextureImpl {
  public:
	Cubemap(NotNull<IRenderDevice*> render_device, CubemapLayers const& layers, Flags const flags) : TextureImpl(render_device, layers, flags) {}

  private:
	[[nodiscard]] auto get_size() const -> glm::ivec2 final { return do_get_size(); }
	[[nodiscard]] auto get_descriptor_info(std::uint32_t binding) const -> DescriptorInfo final { return do_get_descriptor_info(sampler, binding); }
};
} // namespace levk
