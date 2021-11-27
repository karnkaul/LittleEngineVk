#include <core/utils/expect.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
using sv = std::string_view;
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, Extent2D size, Span<ImgView const> bitmaps) {
	auto info = Image::info(size, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format, false);
	if (bitmaps.size() > 1) {
		info.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		info.createInfo.arrayLayers = (u32)bitmaps.size();
		info.view.type = vk::ImageViewType::eCube;
	}
	Image ret(&vram, info);
	out_future = vram.copy(bitmaps, ret, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
	return ret;
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

Texture::Texture(not_null<VRAM*> vram) : m_vram(vram) {}

bool Texture::construct(CreateInfo const& info) {
	Storage storage;
	if (construct(info, storage)) {
		m_storage = std::move(storage);
		return true;
	}
	return false;
}

bool Texture::assign(Image&& image, Type type, Payload payload, vk::Sampler sampler) {
	if (!sampler) { sampler = m_storage.data.sampler; }
	EXPECT(image.image() && image.view() && (image.usage() & usage_v) == usage_v && sampler);
	if (image.image() && image.view() && (image.usage() & usage_v) == usage_v && sampler) {
		wait();
		m_storage.image.emplace(std::move(image));
		m_storage.data.type = type;
		m_storage.data.payload = payload;
		m_storage.data.format = image.viewFormat();
		m_storage.data.size = image.extent2D();
		m_storage.data.imageView = image.view();
		return true;
	}
	return false;
}

bool Texture::blit(CommandBuffer cb, Image const& src, vk::Filter filter) {
	// EXPECT(valid());
	if (!valid()) {
		g_log.log(lvl::debug, 2, "[{}] Creating image from src", g_name);
		auto info = Image::info(src.extent2D(), usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, src.imageFormat(), false);
		m_storage.image.emplace(m_vram, info);
	}
	if (valid()) {
		g_log.log(lvl::debug, 2, "[{}] Initiating blit", g_name);
		auto const srcLayout = m_vram->m_device->m_layouts.get(src.image());
		auto const thisLayout = m_vram->m_device->m_layouts.get(image().image());
		auto layout = [](vIL l) { return l == vIL::eUndefined ? vIL::eShaderReadOnlyOptimal : l; };
		m_vram->m_device->m_layouts.transition(cb, src.image(), vIL::eTransferSrcOptimal, LayoutStages::colourTransfer());
		m_vram->m_device->m_layouts.transition(cb, image().image(), vIL::eTransferDstOptimal, LayoutStages::colourTransfer());
		VRAM::blit(cb, src, *m_storage.image, filter);
		m_vram->m_device->m_layouts.transition(cb, src.image(), layout(srcLayout), LayoutStages::topBottom());
		m_vram->m_device->m_layouts.transition(cb, image().image(), layout(thisLayout), LayoutStages::topBottom());
		return true;
	}
	return false;
}

bool Texture::valid() const noexcept { return m_storage.image.has_value(); }

bool Texture::busy() const { return valid() && m_storage.transfer.busy(); }

bool Texture::ready() const { return valid() && m_storage.transfer.ready(); }

void Texture::wait() const { m_storage.transfer.wait(); }

Texture::Data const& Texture::data() const noexcept { return m_storage.data; }

Image const& Texture::image() const {
	ENSURE(m_storage.image.has_value(), "Invalid image");
	return *m_storage.image;
}

Texture::Img Texture::img(Span<std::byte const> bytes) { return utils::bmpBytes(bytes); }

Cubemap Texture::unitCubemap(Colour colour) {
	Cubemap ret;
	ret.size = {1, 1};
	ret.compressed = false;
	for (auto& b : ret.bytes) { b = {colour.r.value, colour.g.value, colour.b.value, colour.a.value}; }
	return ret;
}

bool Texture::construct(CreateInfo const& info, Storage& out_storage) {
	if (Device::default_v(info.sampler)) { return false; }
	auto const* pImg = std::get_if<Img>(&info.data);
	auto const* pCube = std::get_if<Cube>(&info.data);
	auto const* pBmp = std::get_if<Bitmap>(&info.data);
	auto const* pCmp = std::get_if<Cubemap>(&info.data);
	if ((!pBmp || pBmp->bytes.empty()) && (!pImg || pImg->empty()) && !pCube && !pCmp) { return false; }
	if (pCube) {
		if (std::any_of(pCube->begin(), pCube->end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	} else if (pCmp) {
		if (std::any_of(pCmp->bytes.begin(), pCmp->bytes.end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	}
	out_storage.data.sampler = info.sampler;
	vk::Format fallback;
	ktl::fixed_vector<ImgView, 6> bmps;
	ktl::fixed_vector<utils::STBImg, 6> stbimgs;
	auto checkSize = [](Extent2D size, auto const& bytes) {
		if (std::size_t(size.x * size.y) * Bitmap::channels != bytes.size()) {
			ENSURE(false, "Invalid Raw image size/dimensions");
			return false;
		}
		return true;
	};
	auto pushBytes = [&stbimgs, &bmps, &checkSize](auto const& bytes, bool stb, Extent2D size = {}) {
		if (stb) {
			stbimgs.push_back(utils::STBImg(bytes));
			bmps.push_back(stbimgs.back().bytes);
		} else {
			if (!checkSize(size, bytes)) { return false; }
			bmps.push_back(bytes);
		}
		return true;
	};
	if (pCube || pImg) {
		if (pCube) {
			for (auto const& bytes : *pCube) { pushBytes(bytes, true); }
			out_storage.data.type = Type::eCube;
		} else {
			pushBytes(*pImg, true);
			out_storage.data.type = Type::e2D;
		}
		out_storage.data.size = stbimgs.back().size;
		fallback = info.payload == Payload::eColour ? Image::srgb_v : Image::linear_v;
	} else {
		if (pBmp) {
			if (!pushBytes(pBmp->bytes, false, pBmp->size)) { return false; }
			out_storage.data.type = Type::e2D;
		} else {
			for (auto bytes : pCmp->bytes) {
				if (!pushBytes(bytes, false, pCmp->size)) { return false; }
			}
			out_storage.data.type = Type::eCube;
		}
		out_storage.data.size = pBmp ? pBmp->size : pCmp->size;
		fallback = Image::linear_v;
	}
	vk::Format const format = info.forceFormat.value_or(fallback);
	out_storage.image = load(*m_vram, out_storage.transfer, format, out_storage.data.size, bmps);
	out_storage.data.format = format;
	out_storage.data.payload = info.payload;
	out_storage.data.imageView = out_storage.image->view();
	return true;
}
} // namespace le::graphics
