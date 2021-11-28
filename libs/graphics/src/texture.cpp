#include <core/utils/expect.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
using sv = std::string_view;
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, Extent2D extent, Span<ImgView const> bitmaps) {
	auto info = Image::info(extent, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format, false);
	if (bitmaps.size() > 1) {
		info.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		info.createInfo.arrayLayers = (u32)bitmaps.size();
		info.view.type = vk::ImageViewType::eCube;
	}
	Image ret(&vram, info);
	out_future = vram.copy(bitmaps, ret, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
	return ret;
}

Image::CreateInfo imageInfo(Extent2D extent, vk::Format format) noexcept {
	return Image::info(extent, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format, false);
}

bool checkSize(Extent2D size, Span<u8 const> bytes) noexcept {
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
	ret.maxLod = 0.0f;
	return ret;
}

Sampler::Sampler(not_null<Device*> device, vk::SamplerCreateInfo const& info) : m_device(device) { m_sampler = makeDeferred<vk::Sampler>(device, info); }
Sampler::Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip) : Sampler(device, info(minMag, mip)) {}

BmpBytes Texture::bmpBytes(Span<std::byte const> bytes) { return utils::bmpBytes(bytes); }

Cubemap Texture::unitCubemap(Colour colour) {
	Cubemap ret;
	ret.size = {1, 1};
	ret.compressed = false;
	for (auto& b : ret.bytes) { b = {colour.r.value, colour.g.value, colour.b.value, colour.a.value}; }
	return ret;
}

Texture::Texture(not_null<VRAM*> vram, vk::Sampler sampler, Colour colour, Extent2D extent, Payload payload)
	: m_image(vram, imageInfo(extent, Image::linear_v)), m_sampler(sampler), m_vram(vram) {
	Bitmap bmp;
	bmp.bytes.reserve(std::size_t(extent.x * extent.y));
	for (u32 i = 0; i < extent.x; ++i) {
		for (u32 j = 0; j < extent.y; ++j) { utils::append(bmp.bytes, colour); }
	}
	bmp.size = extent;
	construct(std::move(bmp), payload);
}

bool Texture::construct(Bitmap const& bitmap, Payload payload, vk::Format format) {
	if (!checkSize(bitmap.size, bitmap.bytes)) { return false; }
	constructImpl(ImgView(bitmap.bytes), bitmap.size, payload, format);
	return true;
}

bool Texture::construct(BmpBytes const& bytes, Payload payload, vk::Format format) {
	utils::STBImg stbimg(bytes);
	if (!checkSize(stbimg.size, stbimg.bytes)) { return false; }
	constructImpl(ImgView(stbimg.bytes), stbimg.size, payload, format);
	return true;
}

bool Texture::construct(Cubemap const& cubemap, Payload payload, vk::Format format) {
	if (std::any_of(cubemap.bytes.begin(), cubemap.bytes.end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	ktl::fixed_vector<ImgView, 6> bmps;
	for (auto bytes : cubemap.bytes) {
		if (!checkSize(cubemap.size, bytes)) { return false; }
		bmps.push_back(bytes);
	}
	constructImpl(bmps, cubemap.size, payload, format);
	return true;
}

bool Texture::construct(CubeBytes const& cubemap, Payload payload, vk::Format format) {
	if (std::any_of(cubemap.begin(), cubemap.end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	ktl::fixed_vector<ImgView, 6> bmps;
	ktl::fixed_vector<utils::STBImg, 6> stbimgs;
	Extent2D extent{};
	for (auto const& bytes : cubemap) {
		stbimgs.push_back(utils::STBImg(bytes));
		if (!checkSize(stbimgs.back().size, stbimgs.back().bytes)) { return false; }
		if (extent == Extent2D()) {
			extent = stbimgs.back().size;
		} else if (extent != stbimgs.back().size) {
			return false;
		}
		bmps.push_back(stbimgs.back().bytes);
	}
	constructImpl(bmps, extent, payload, format);
	return true;
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
		auto info = Image::info(extent, usage_v, vIAFB::eColor, VMA_MEMORY_USAGE_GPU_ONLY, m_image.imageFormat(), false);
		Image image(m_vram, info);
		std::swap(m_image, image);
		return blit(cb, makeRenderTarget(image));
	}
	return false;
}

bool Texture::blit(CommandBuffer cb, Texture const& src, vk::Filter filter) { return blit(cb, src.image(), filter); }

bool Texture::blit(CommandBuffer cb, Image const& src, vk::Filter filter) {
	wait();
	return m_vram->blit(cb, src, m_image, filter);
}

bool Texture::blit(CommandBuffer cb, RenderTarget const& src, vk::Filter filter) {
	EXPECT(src.image);
	if (src.image) {
		wait();
		RenderTarget const dst = renderTarget();
		auto const srcLayout = m_vram->m_device->m_layouts.get(src.image);
		auto const thisLayout = m_vram->m_device->m_layouts.get(dst.image);
		auto layout = [](vIL l) { return l == vIL::eUndefined ? vIL::eShaderReadOnlyOptimal : l; };
		m_vram->m_device->m_layouts.transition(cb, src.image, vIL::eTransferSrcOptimal, LayoutStages::colourTransfer());
		m_vram->m_device->m_layouts.transition(cb, dst.image, vIL::eTransferDstOptimal, LayoutStages::colourTransfer());
		auto const ret = m_vram->blit(cb, {src, dst}, filter);
		m_vram->m_device->m_layouts.transition(cb, src.image, layout(srcLayout), LayoutStages::allCommands());
		m_vram->m_device->m_layouts.transition(cb, dst.image, layout(thisLayout), LayoutStages::allCommands());
		return ret;
	}
	return false;
}

void Texture::constructImpl(Span<ImgView const> bmps, Extent2D extent, Payload payload, vk::Format format) {
	m_payload = payload;
	m_type = bmps.size() > 1 ? Type::eCube : Type::e2D;
	m_image = load(*m_vram, m_transfer, format, extent, bmps);
}
} // namespace le::graphics
