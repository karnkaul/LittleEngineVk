#include <graphics/context/device.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
using sv = std::string_view;
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, Extent2D size, Span<ImgView const> bitmaps) {
	Image::CreateInfo imageInfo;
	imageInfo.queueFlags = QFlags(QType::eTransfer) | QType::eGraphics;
	imageInfo.createInfo.format = format;
	imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	if (bitmaps.size() > 1) { imageInfo.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible; }
	imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageInfo.createInfo.extent = vk::Extent3D((u32)size.x, (u32)size.y, 1);
	imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.createInfo.imageType = vk::ImageType::e2D;
	imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.createInfo.mipLevels = 1;
	imageInfo.createInfo.arrayLayers = (u32)bitmaps.size();
	Image ret(&vram, imageInfo);
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
		fallback = info.payload == Payload::eColour ? srgb : linear;
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
		fallback = linear;
	}
	vk::Format const format = info.forceFormat.value_or(fallback);
	out_storage.image = load(*m_vram, out_storage.transfer, format, out_storage.data.size, bmps);
	out_storage.data.format = format;
	Device& d = *m_vram->m_device;
	vk::ImageViewType const type = out_storage.data.type == Type::eCube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
	out_storage.view = {&d, d.makeImageView(out_storage.image->image(), out_storage.data.format, vk::ImageAspectFlagBits::eColor, type)};
	out_storage.data.imageView = out_storage.view;
	return true;
}
} // namespace le::graphics
