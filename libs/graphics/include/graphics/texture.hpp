#pragma once
#include <core/colour.hpp>
#include <glm/vec2.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/utils/deferred.hpp>
#include <ktl/fixed_vector.hpp>

namespace le::graphics {
class Sampler {
  public:
	using MinMag = TPair<vk::Filter>;
	static vk::SamplerCreateInfo info(MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	Sampler(not_null<Device*> device, vk::SamplerCreateInfo info);
	Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	vk::Sampler sampler() const noexcept { return m_sampler; }

  private:
	Deferred<vk::Sampler> m_sampler;
	not_null<Device*> m_device;
};

class Texture {
  public:
	enum class Type { e2D, eCube };
	enum class Payload { eColour, eData };

	static constexpr vk::ImageUsageFlags usage_v = vIUFB::eSampled | vIUFB::eTransferSrc | vIUFB::eTransferDst;

	static constexpr Extent2D default_extent_v = {32U, 32U};

	static Cubemap unitCubemap(Colour colour);
	static RenderTarget makeRenderTarget(Image const& image) noexcept { return {image.image(), image.view(), image.extent2D(), image.viewFormat()}; }

	Texture(not_null<VRAM*> vram, vk::Sampler sampler, Colour colour = colours::white, Extent2D extent = default_extent_v, Payload payload = Payload::eColour);

	bool construct(Bitmap const& bitmap, Payload payload = Payload::eColour, vk::Format format = Image::linear_v);
	bool construct(ImageData img, Payload payload = Payload::eColour, vk::Format format = Image::srgb_v);
	bool construct(Cubemap const& cubemap, Payload payload = Payload::eColour, vk::Format format = Image::linear_v);
	bool construct(Span<ImageData const> cubeImgs, Payload payload = Payload::eColour, vk::Format format = Image::srgb_v);

	bool changeSampler(vk::Sampler sampler);
	bool assign(Image&& image, Type type = Type::e2D, Payload payload = Payload::eColour);
	bool resize(CommandBuffer cb, Extent2D extent);
	bool blit(CommandBuffer cb, Texture const& src, vk::Filter filter = vk::Filter::eLinear);
	bool blit(CommandBuffer cb, Image const& src, vk::Filter filter = vk::Filter::eLinear);
	bool copy(CommandBuffer cb, Image const& src);

	bool busy() const { return m_transfer.busy(); }
	bool ready() const { return m_transfer.ready(); }
	void wait() const { m_transfer.wait(); }

	Extent2D extent() const noexcept { return m_image.extent2D(); }
	vk::Sampler sampler() const noexcept { return m_sampler; }
	Image const& image() const noexcept { return m_image; }
	RenderTarget renderTarget() const noexcept { return makeRenderTarget(m_image); }
	Payload payload() const noexcept { return m_payload; }
	Type type() const noexcept { return m_type; }

  private:
	bool constructImpl(Span<BmpView const> bmps, Extent2D extent, Payload payload, vk::Format format);
	bool constructImpl(VRAM::Images&& imgs, Payload payload, vk::Format format);

	Image m_image;
	VRAM::Future m_transfer;
	vk::Sampler m_sampler;
	Payload m_payload{};
	Type m_type{};
	not_null<VRAM*> m_vram;
};
} // namespace le::graphics
