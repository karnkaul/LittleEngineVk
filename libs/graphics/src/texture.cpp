#include <graphics/context/device.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
using sv = std::string_view;
Image load(VRAM& vram, VRAM::Future& out_future, vk::Format format, glm::ivec2 size, View<View<std::byte>> bytes, [[maybe_unused]] sv name) {
	Image::CreateInfo imageInfo;
	imageInfo.queueFlags = QType::eTransfer | QType::eGraphics;
	imageInfo.createInfo.format = format;
	imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	if (bytes.size() > 1) {
		imageInfo.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	}
	imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageInfo.createInfo.extent = vk::Extent3D((u32)size.x, (u32)size.y, 1);
	imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.createInfo.imageType = vk::ImageType::e2D;
	imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.createInfo.mipLevels = 1;
	imageInfo.createInfo.arrayLayers = (u32)bytes.size();
#if defined(LEVK_VKRESOURCE_NAMES)
	imageInfo.name = std::string(name);
#endif
	Image ret(vram, imageInfo);
	out_future = vram.copy(bytes, ret, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
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

Sampler::Sampler(Device& device, vk::SamplerCreateInfo const& info) : m_device(device) {
	m_sampler = device.device().createSampler(info);
}

Sampler::Sampler(Device& device, MinMag minMag, vk::SamplerMipmapMode mip) : Sampler(device, info(minMag, mip)) {
}

Sampler::Sampler(Sampler&& rhs) : m_sampler(std::exchange(rhs.m_sampler, vk::Sampler())), m_device(rhs.m_device) {
}

Sampler& Sampler::operator=(Sampler&& rhs) {
	if (&rhs != this) {
		destroy();
		m_sampler = std::exchange(rhs.m_sampler, vk::Sampler());
		m_device = rhs.m_device;
	}
	return *this;
}

Sampler::~Sampler() {
	destroy();
}

void Sampler::destroy() {
	if (!Device::default_v(m_sampler)) {
		Device& d = m_device;
		d.defer([&d, s = m_sampler]() { d.device().destroySampler(s); });
	}
}

Texture::Texture(std::string name, VRAM& vram) : m_name(std::move(name)), m_vram(vram) {
}
Texture::Texture(Texture&& rhs) : m_name(std::move(rhs.m_name)), m_vram(rhs.m_vram), m_storage(std::exchange(rhs.m_storage, Storage())) {
}
Texture& Texture::operator=(Texture&& rhs) {
	if (&rhs != this) {
		destroy();
		m_name = std::move(rhs.m_name);
		m_storage = std::exchange(rhs.m_storage, Storage());
		m_vram = rhs.m_vram;
	}
	return *this;
}
Texture::~Texture() {
	destroy();
}

bool Texture::construct(CreateInfo const& info) {
	Storage storage;
	if (construct(info, storage)) {
		destroy();
		m_storage = std::move(storage);
		return true;
	}
	return false;
}

void Texture::destroy() {
	wait();
	Device& d = m_vram.get().m_device;
	d.defer([&d, data = m_storage.data, r = m_storage.raw]() mutable {
		d.destroy(data.imageView);
		for (auto const& img : r.imgs) {
			utils::release(img);
		}
	});
	m_storage = {};
}

bool Texture::valid() const {
	return m_storage.image.has_value();
}

bool Texture::busy() const {
	return valid() && m_storage.transfer.busy();
}

bool Texture::ready() const {
	return valid() && m_storage.transfer.ready(true);
}

void Texture::wait() const {
	m_storage.transfer.wait();
}

Texture::Data const& Texture::data() const noexcept {
	return m_storage.data;
}

Image const& Texture::image() const {
	ENSURE(m_storage.image.has_value(), "Invalid image");
	return *m_storage.image;
}

bool Texture::construct(CreateInfo const& info, Storage& out_storage) {
	if (Device::default_v(info.sampler)) {
		return false;
	}
	Img const* pImg = std::get_if<Img>(&info.data);
	Cubemap const* pCube = std::get_if<Cubemap>(&info.data);
	Raw const* pRaw = std::get_if<Raw>(&info.data);
	if ((!pRaw || pRaw->bytes.empty()) && (!pImg || pImg->bytes.empty()) && !pCube) {
		return false;
	}
	if (pCube) {
		if (std::any_of(pCube->bytes.begin(), pCube->bytes.end(), [](bytearray const& b) { return b.empty(); })) {
			return false;
		}
	}
	out_storage.data.sampler = info.sampler;
	if (pCube || pImg) {
		if (pCube) {
			for (auto const& bytes : pCube->bytes) {
				out_storage.raw.imgs.push_back(utils::decompress(bytes));
				out_storage.raw.bytes.push_back(out_storage.raw.imgs.back().bytes);
				out_storage.data.type = Type::eCube;
			}
		} else {
			out_storage.raw.imgs.push_back(utils::decompress(pImg->bytes));
			out_storage.raw.bytes.push_back(out_storage.raw.imgs.back().bytes);
			out_storage.data.type = Type::e2D;
		}
		out_storage.data.size = {out_storage.raw.imgs.back().width, out_storage.raw.imgs.back().height};
	} else {
		if (std::size_t(pRaw->size.x * pRaw->size.y) * 4 /*channels*/ != pRaw->bytes.size()) {
			ENSURE(false, "Invalid Raw image size/dimensions");
			return false;
		}
		out_storage.data.size = pRaw->size;
		out_storage.raw.bytes.push_back(pRaw->bytes.back());
		out_storage.data.type = Type::e2D;
	}
	out_storage.image = load(m_vram, out_storage.transfer, info.format, out_storage.data.size, out_storage.raw.bytes, m_name);
	out_storage.data.format = info.format;
	Device& d = m_vram.get().m_device;
	vk::ImageViewType const type = out_storage.data.type == Type::eCube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
	out_storage.data.imageView = d.createImageView(out_storage.image->image(), out_storage.data.format, vk::ImageAspectFlagBits::eColor, type);
	return true;
}
} // namespace le::graphics
