#include <levk/core/utils/expect.hpp>
#include <levk/graphics/command_buffer.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/device/device.hpp>
#include <levk/graphics/render/renderer.hpp>
#include <levk/graphics/texture.hpp>
#include <levk/graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
template <typename T>
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, Extent2D extent, T&& bitmaps, bool mips) {
	auto info = Image::textureInfo(extent, format, mips);
	if (bitmaps.size() > 1) {
		info.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		info.createInfo.arrayLayers = (u32)bitmaps.size();
		info.view.type = vk::ImageViewType::eCube;
	}
	Image ret(&vram, info);
	out_future = vram.copyAsync(std::forward<T>(bitmaps), ret, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
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
	ret.anisotropyEnable = true;
	return ret;
}

Sampler::Sampler(not_null<Device*> device, vk::SamplerCreateInfo info) : m_device(device) {
	info.anisotropyEnable &= device->physicalDevice().features.samplerAnisotropy;
	if (info.anisotropyEnable) {
		if (info.maxAnisotropy == 0.0f) {
			info.maxAnisotropy = device->maxAnisotropy();
		} else {
			info.maxAnisotropy = std::min(info.maxAnisotropy, device->maxAnisotropy());
		}
	}
	m_sampler = m_sampler.make(device->makeSampler(info), device);
}

Sampler::Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip) : Sampler(device, info(minMag, mip)) {}

Cubemap Texture::unitCubemap(Colour colour) {
	Cubemap ret;
	ret.extent = {1, 1};
	for (auto& b : ret.bytes) { b = {colour.r.value, colour.g.value, colour.b.value, colour.a.value}; }
	return ret;
}

Texture::Texture(not_null<VRAM*> vram, vk::Sampler sampler, Colour colour, Extent2D extent, Payload payload, bool mips)
	: m_image(vram, Image::textureInfo(extent, Image::linear_v, mips)), m_sampler(sampler), m_vram(vram) {
	Bitmap bmp;
	bmp.bytes.reserve(std::size_t(extent.x * extent.y));
	for (u32 i = 0; i < extent.x; ++i) {
		for (u32 j = 0; j < extent.y; ++j) { utils::append(bmp.bytes, colour); }
	}
	bmp.extent = extent;
	construct(std::move(bmp), payload, Image::linear_v, mips);
}

bool Texture::construct(Bitmap const& bitmap, Payload payload, vk::Format format, bool mips) {
	if (!checkSize(bitmap.extent, bitmap.bytes)) { return false; }
	constructImpl(BmpView(bitmap.bytes), bitmap.extent, payload, format, mips);
	return true;
}

bool Texture::construct(ImageData img, Payload payload, vk::Format format, bool mips) {
	VRAM::Images imgs;
	imgs.push_back(utils::STBImg(img));
	return constructImpl(std::move(imgs), payload, format, mips);
}

bool Texture::construct(Cubemap const& cubemap, Payload payload, vk::Format format, bool mips) {
	if (std::any_of(cubemap.bytes.begin(), cubemap.bytes.end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	ktl::fixed_vector<BmpView, 6> bmps;
	for (auto const& bytes : cubemap.bytes) { bmps.push_back(bytes); }
	return constructImpl(bmps, cubemap.extent, payload, format, mips);
}

bool Texture::construct(Span<ImageData const> cubeImgs, Payload payload, vk::Format format, bool mips) {
	if (std::any_of(cubeImgs.begin(), cubeImgs.end(), [](auto const& b) { return b.empty(); })) { return false; }
	VRAM::Images imgs;
	for (auto const& bytes : cubeImgs) { imgs.push_back(utils::STBImg(bytes)); }
	return constructImpl(std::move(imgs), payload, format, mips);
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

Texture::Result Texture::resizeBlit(CommandBuffer cb, Extent2D extent) {
	if (extent == m_image.extent2D()) { return true; }
	EXPECT(extent.x > 0 && extent.y > 0);
	if (extent.x > 0 && extent.y > 0) { return resize(cb, extent, true); }
	return false;
}

Texture::Result Texture::resizeCopy(CommandBuffer cb, Extent2D extent) {
	if (extent == m_image.extent2D()) { return true; }
	EXPECT(extent.x > 0 && extent.y > 0);
	if (extent.x > 0 && extent.y > 0) { return resize(cb, extent, false); }
	return false;
}

bool Texture::blit(CommandBuffer cb, ImageRef const& src, BlitFilter filter) {
	wait();
	return utils::blit(m_vram, cb, src, m_image, filter);
}

Texture::Result Texture::copy(CommandBuffer cb, ImageRef const& src, bool allowResize) {
	wait();
	Result ret;
	if (m_image.extent2D() != src.extent) {
		if (!allowResize || src.extent.x == 0 || src.extent.y == 0) { return {}; }
		ret.scratch.image = std::move(m_image);
		m_image = Image(m_vram, Image::textureInfo(src.extent, m_image.format(), m_image.mipCount() > 1U));
	}
	ret.outcome = utils::copy(m_vram, cb, src, m_image);
	return ret;
}

bool Texture::constructImpl(Span<BmpView const> imgs, Extent2D extent, Payload payload, vk::Format format, bool mips) {
	for (auto const& bytes : imgs) {
		if (!checkSize(extent, bytes)) { return false; }
	}
	m_payload = payload;
	m_type = imgs.size() > 1 ? Type::eCube : Type::e2D;
	m_image = load(*m_vram, m_transfer, format, extent, imgs, mips);
	return true;
}

bool Texture::constructImpl(VRAM::Images&& imgs, Payload payload, vk::Format format, bool mips) {
	if (imgs.empty()) { return false; }
	for (auto const& img : imgs) {
		if (!checkSize(img.extent, img.bytes)) { return false; }
	}
	m_payload = payload;
	m_type = imgs.size() > 1 ? Type::eCube : Type::e2D;
	auto const extent = imgs.front().extent;
	m_image = load(*m_vram, m_transfer, format, extent, std::move(imgs), mips);
	return true;
}

Texture::Result Texture::resize(CommandBuffer cb, Extent2D extent, bool viaBlit) {
	wait();
	Image image(m_vram, Image::textureInfo(extent, m_image.format(), m_image.mipCount() > 1U));
	Result ret;
	if (viaBlit) {
		ret.outcome = utils::blit(m_vram, cb, m_image.ref(), image, BlitFilter::eLinear);
	} else {
		ret.outcome = utils::copy(m_vram, cb, m_image.ref(), image);
	}
	if (ret.outcome) {
		LayerMip lm;
		lm.layer.count = image.layerCount();
		lm.mip.count = image.mipCount();
		utils::Transition{m_vram->m_device, &cb, image.image()}(vIL::eShaderReadOnlyOptimal, lm);
		std::swap(m_image, image);
		ret.scratch.image = std::move(image);
	}
	return ret;
}
} // namespace le::graphics
