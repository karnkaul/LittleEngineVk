#include <graphics/context/device.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
using sv = std::string_view;
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, glm::ivec2 size, View<BMPview> bitmaps) {
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

Sampler::Sampler(not_null<Device*> device, vk::SamplerCreateInfo const& info) : m_device(device) { m_sampler = device->device().createSampler(info); }

Sampler::Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip) : Sampler(device, info(minMag, mip)) {}

Sampler::Sampler(Sampler&& rhs) : m_sampler(std::exchange(rhs.m_sampler, vk::Sampler())), m_device(rhs.m_device) {}

Sampler& Sampler::operator=(Sampler&& rhs) {
	if (&rhs != this) {
		destroy();
		m_sampler = std::exchange(rhs.m_sampler, vk::Sampler());
		m_device = rhs.m_device;
	}
	return *this;
}

Sampler::~Sampler() { destroy(); }

void Sampler::destroy() {
	if (!Device::default_v(m_sampler)) {
		Device& d = *m_device;
		d.defer([&d, s = m_sampler]() { d.device().destroySampler(s); });
	}
}

Texture::Texture(not_null<VRAM*> vram) : m_vram(vram) {}
Texture::Texture(Texture&& rhs) : m_vram(rhs.m_vram), m_storage(std::exchange(rhs.m_storage, Storage())) {}
Texture& Texture::operator=(Texture&& rhs) {
	if (&rhs != this) {
		destroy();
		m_storage = std::exchange(rhs.m_storage, Storage());
		m_vram = rhs.m_vram;
	}
	return *this;
}
Texture::~Texture() { destroy(); }

bool Texture::construct(CreateInfo const& info) {
	Storage storage;
	if (construct(info, storage)) {
		destroy();
		m_storage = std::move(storage);
		return true;
	}
	return false;
}

bool Texture::valid() const noexcept { return m_storage.image.has_value(); }

bool Texture::busy() const { return valid() && m_storage.transfer.busy(); }

bool Texture::ready() const { return valid() && m_storage.transfer.ready(true); }

void Texture::wait() const { m_storage.transfer.wait(); }

Texture::Data const& Texture::data() const noexcept { return m_storage.data; }

Image const& Texture::image() const {
	ENSURE(m_storage.image.has_value(), "Invalid image");
	return *m_storage.image;
}

bool Texture::construct(CreateInfo const& info, Storage& out_storage) {
	if (Device::default_v(info.sampler)) { return false; }
	auto const* pImg = std::get_if<Img>(&info.data);
	auto const* pCube = std::get_if<Cubemap>(&info.data);
	auto const* pRaw = std::get_if<Bitmap>(&info.data);
	if ((!pRaw || pRaw->bytes.empty()) && (!pImg || pImg->empty()) && !pCube) { return false; }
	if (pCube) {
		if (std::any_of(pCube->begin(), pCube->end(), [](Bitmap::type const& b) { return b.empty(); })) { return false; }
	}
	out_storage.data.sampler = info.sampler;
	vk::Format fallback;
	kt::fixed_vector<BMPview, 6> bmps;
	kt::fixed_vector<utils::STBImg, 6> stbimgs;
	if (pCube || pImg) {
		if (pCube) {
			for (auto const& bytes : *pCube) {
				stbimgs.push_back(utils::STBImg(bytes));
				bmps.push_back(stbimgs.back().bytes);
				out_storage.data.type = Type::eCube;
			}
		} else {
			stbimgs.push_back(utils::STBImg(*pImg));
			bmps.push_back(stbimgs.back().bytes);
			out_storage.data.type = Type::e2D;
		}
		out_storage.data.size = {stbimgs.back().size.x, stbimgs.back().size.y};
		fallback = srgb;
	} else {
		if (std::size_t(pRaw->size.x * pRaw->size.y) * 4 /*channels*/ != pRaw->bytes.size()) {
			ENSURE(false, "Invalid Raw image size/dimensions");
			return false;
		}
		out_storage.data.size = pRaw->size;
		bmps.push_back(pRaw->bytes);
		out_storage.data.type = Type::e2D;
		fallback = linear;
	}
	vk::Format const format = info.forceFormat.value_or(fallback);
	out_storage.image = load(*m_vram, out_storage.transfer, format, out_storage.data.size, bmps);
	out_storage.data.format = format;
	Device& d = *m_vram->m_device;
	vk::ImageViewType const type = out_storage.data.type == Type::eCube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
	out_storage.data.imageView = d.makeImageView(out_storage.image->image(), out_storage.data.format, vk::ImageAspectFlagBits::eColor, type);
	return true;
}

void Texture::destroy() {
	wait();
	Device& d = *m_vram->m_device;
	d.defer([&d, data = m_storage.data]() mutable { d.destroy(data.imageView); });
	m_storage = {};
}
} // namespace le::graphics
