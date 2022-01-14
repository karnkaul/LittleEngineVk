#pragma once
#include <core/colour.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>
#include <levk/graphics/bitmap.hpp>
#include <levk/graphics/context/vram.hpp>

namespace le::graphics {
class Sampler {
  public:
	using MinMag = TPair<vk::Filter>;
	static vk::SamplerCreateInfo info(MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	Sampler(not_null<Device*> device, vk::SamplerCreateInfo info);
	Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	vk::Sampler sampler() const noexcept { return m_sampler; }

  private:
	Defer<vk::Sampler> m_sampler;
	not_null<Device*> m_device;
};

class Texture {
  public:
	enum class Type { e2D, eCube };
	enum class Payload { eColour, eData };

	using Result = VRAM::Op<>;

	static constexpr vk::ImageUsageFlags usage_v = vIUFB::eSampled | vIUFB::eTransferSrc | vIUFB::eTransferDst;

	static constexpr Extent2D default_extent_v = {32U, 32U};

	static Cubemap unitCubemap(Colour colour);

	Texture(not_null<VRAM*> vram, vk::Sampler sm, Colour cl = colours::white, Extent2D ex = default_extent_v, Payload pl = Payload::eColour, bool mips = true);

	bool construct(Bitmap const& bitmap, Payload payload = Payload::eColour, vk::Format format = Image::linear_v, bool mips = true);
	bool construct(ImageData img, Payload payload = Payload::eColour, vk::Format format = Image::srgb_v, bool mips = true);
	bool construct(Cubemap const& cubemap, Payload payload = Payload::eColour, vk::Format format = Image::linear_v, bool mips = true);
	bool construct(Span<ImageData const> cubeImgs, Payload payload = Payload::eColour, vk::Format format = Image::srgb_v, bool mips = true);

	bool changeSampler(vk::Sampler sampler);
	bool assign(Image&& image, Type type = Type::e2D, Payload payload = Payload::eColour);
	Result resizeBlit(CommandBuffer cb, Extent2D extent);
	Result resizeCopy(CommandBuffer cb, Extent2D extent);
	bool blit(CommandBuffer cb, ImageRef const& src, BlitFilter filter = BlitFilter::eLinear);
	Result copy(CommandBuffer cb, ImageRef const& src, bool allowResize);

	bool busy() const { return m_transfer.busy(); }
	bool ready() const { return m_transfer.ready(); }
	void wait() const { m_transfer.wait(); }

	vk::Sampler sampler() const noexcept { return m_sampler; }
	Image const& image() const noexcept { return m_image; }
	Payload payload() const noexcept { return m_payload; }
	Type type() const noexcept { return m_type; }

  private:
	bool constructImpl(Span<BmpView const> bmps, Extent2D extent, Payload payload, vk::Format format, bool mips);
	bool constructImpl(VRAM::Images&& imgs, Payload payload, vk::Format format, bool mips);
	Result resize(CommandBuffer cb, Extent2D extent, bool viaBlit);

	Image m_image;
	VRAM::Future m_transfer;
	vk::Sampler m_sampler;
	Payload m_payload{};
	Type m_type{};
	not_null<VRAM*> m_vram;
};
} // namespace le::graphics
