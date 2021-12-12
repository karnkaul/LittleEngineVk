#include <core/utils/expect.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
template <typename T>
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, Extent2D extent, T&& bitmaps) {
	auto info = Image::textureInfo(extent, format);
	if (bitmaps.size() > 1) {
		info.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		info.createInfo.arrayLayers = (u32)bitmaps.size();
		info.view.type = vk::ImageViewType::eCube;
	}
	Image ret(&vram, info);
	out_future = vram.copy(std::forward<T>(bitmaps), ret, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
	return ret;
}

template <typename T>
bool checkSize(Extent2D size, T const& bytes) noexcept {
	if (std::size_t(size.x * size.y) * Bitmap::channels != bytes.size()) { return false; }
	return true;
}
} // namespace

vk::SamplerCreateInfo Sampler::info(MinMag minMag, vk::SamplerMipmapMode mip) {
	vk::SamplerCreateInfo ret;
	ret.magFilter = minMag.first;
	ret.minFilter = minMag.second;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = vk::SamplerAddressMode::eRepeat;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	ret.unnormalizedCoordinates = false;
	ret.compareEnable = false;
	ret.compareOp = vk::CompareOp::eAlways;
	ret.mipmapMode = mip;
	ret.mipLodBias = 0.0f;
	ret.minLod = 0.0f;
	ret.maxLod = VK_LOD_CLAMP_NONE;
	return ret;
}

Sampler::Sampler(not_null<Device*> device, vk::SamplerCreateInfo const& info) : m_device(device) { m_sampler = makeDeferred<vk::Sampler>(device, info); }
Sampler::Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip) : Sampler(device, info(minMag, mip)) {}

Cubemap Texture::unitCubemap(Colour colour) {
	Cubemap ret;
	ret.extent = {1, 1};
	for (auto& b : ret.bytes) { b = {colour.r.value, colour.g.value, colour.b.value, colour.a.value}; }
	return ret;
}

Texture::Texture(not_null<VRAM*> vram, vk::Sampler sampler, Colour colour, Extent2D extent, Payload payload)
	: m_image(vram, Image::textureInfo(extent, Image::linear_v)), m_sampler(sampler), m_vram(vram) {
	Bitmap bmp;
	bmp.bytes.reserve(std::size_t(extent.x * extent.y));
	for (u32 i = 0; i < extent.x; ++i) {
		for (u32 j = 0; j < extent.y; ++j) { utils::append(bmp.bytes, colour); }
	}
	bmp.extent = extent;
	construct(std::move(bmp), payload);
}

bool Texture::construct(Bitmap const& bitmap, Payload payload, vk::Format format) {
	if (!checkSize(bitmap.extent, bitmap.bytes)) { return false; }
	constructImpl(BmpView(bitmap.bytes), bitmap.extent, payload, format);
	return true;
}

bool Texture::construct(ImageData img, Payload payload, vk::Format format) {
	VRAM::Images imgs;
	imgs.push_back(utils::STBImg(img));
	return constructImpl(std::move(imgs), payload, format);
}

bool Texture::construct(Cubemap const& cubemap, Payload payload, vk::Format format) {
	if (std::any_of(cubemap.bytes.begin(), cubemap.bytes.end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	ktl::fixed_vector<BmpView, 6> bmps;
	for (auto const& bytes : cubemap.bytes) { bmps.push_back(bytes); }
	return constructImpl(bmps, cubemap.extent, payload, format);
}

bool Texture::construct(Span<ImageData const> cubeImgs, Payload payload, vk::Format format) {
	if (std::any_of(cubeImgs.begin(), cubeImgs.end(), [](auto const& b) { return b.empty(); })) { return false; }
	VRAM::Images imgs;
	for (auto const& bytes : cubeImgs) { imgs.push_back(utils::STBImg(bytes)); }
	return constructImpl(std::move(imgs), payload, format);
}

bool Texture::changeSampler(vk::Sampler sampler) {
	EXPECT(sampler);
	if (sampler) {
		m_sampler = sampler;
		return true;
	}
	return false;
}

bool Texture::assign(Image&& image, Type type, Payload payload) {
	EXPECT(image.image() && image.view() && (image.usage() & usage_v) == usage_v);
	if (image.image() && image.view() && (image.usage() & usage_v) == usage_v) {
		wait();
		m_image = std::move(image);
		m_type = type;
		m_payload = payload;
		return true;
	}
	return false;
}

bool Texture::resize(CommandBuffer cb, Extent2D extent) {
	EXPECT(extent.x > 0 && extent.y > 0);
	if (extent.x > 0 && extent.y > 0) {
		Image image(m_vram, Image::textureInfo(extent, m_image.imageFormat()));
		std::swap(m_image, image);
		return blit(cb, image);
	}
	return false;
}

bool Texture::blit(CommandBuffer cb, Texture const& src, vk::Filter filter) { return blit(cb, src.image(), filter); }

bool Texture::blit(CommandBuffer cb, Image const& src, vk::Filter filter) {
	wait();
	return utils::blit(m_vram, cb, src, m_image, filter);
}

bool Texture::copy(CommandBuffer cb, Image const& src) {
	wait();
	if (m_image.extent2D() != src.extent2D()) { resize(cb, src.extent2D()); }
	return utils::copy(m_vram, cb, src, m_image);
}

bool Texture::constructImpl(Span<BmpView const> imgs, Extent2D extent, Payload payload, vk::Format format) {
	for (auto const& bytes : imgs) {
		if (!checkSize(extent, bytes)) { return false; }
	}
	m_payload = payload;
	m_type = imgs.size() > 1 ? Type::eCube : Type::e2D;
	m_image = load(*m_vram, m_transfer, format, extent, imgs);
	return true;
}

bool Texture::constructImpl(VRAM::Images&& imgs, Payload payload, vk::Format format) {
	if (imgs.empty()) { return false; }
	for (auto const& img : imgs) {
		if (!checkSize(img.extent, img.bytes)) { return false; }
	}
	m_payload = payload;
	m_type = imgs.size() > 1 ? Type::eCube : Type::e2D;
	auto const extent = imgs.front().extent;
	m_image = load(*m_vram, m_transfer, format, extent, std::move(imgs));
	return true;
}
} // namespace le::graphics
