#include <core/utils.hpp>
#include <graphics/context/device.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
std::future<void> load(VRAM& vram, View<Image>& out_image, vk::Format format, glm::ivec2 size, Span<Span<u8>> bytes, [[maybe_unused]] std::string_view name) {
	Image::CreateInfo imageInfo;
	imageInfo.queueFlags = QType::eTransfer | QType::eGraphics;
	imageInfo.createInfo.format = format;
	imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	if (bytes.extent > 1) {
		imageInfo.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	}
	imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageInfo.createInfo.extent = vk::Extent3D((u32)size.x, (u32)size.y, 1);
	imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.createInfo.imageType = vk::ImageType::e2D;
	imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.createInfo.mipLevels = 1;
	imageInfo.createInfo.arrayLayers = (u32)bytes.extent;
#if defined(LEVK_VKRESOURCE_NAMES)
	imageInfo.name = std::string(name);
#endif
	out_image = vram.construct(imageInfo);
	return vram.copy(bytes, out_image, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
}
} // namespace

Texture::Texture(std::string name, VRAM& vram) : m_name(std::move(name)), m_vram(vram) {
}
Texture::Texture(Texture&& rhs) : m_name(std::move(rhs.m_name)), m_storage(std::exchange(rhs.m_storage, Storage())), m_vram(rhs.m_vram) {
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
	destroy();
	if (default_v(info.sampler)) {
		return false;
	}
	Compressed const* pComp = std::get_if<Compressed>(&info.data);
	Raw const* pRaw = std::get_if<Raw>(&info.data);
	if ((!pRaw || pRaw->bytes.empty()) && (!pComp || pComp->bytes.empty())) {
		return false;
	}
	m_storage.data.sampler = info.sampler;
	m_storage.data.type = info.type;
	if (pComp) {
		for (auto const& bytes : pComp->bytes) {
			m_storage.raw.imgs.push_back(utils::decompress(bytes));
			m_storage.raw.bytes.push_back(m_storage.raw.imgs.back().bytes);
		}
		m_storage.data.size = {m_storage.raw.imgs.back().width, m_storage.raw.imgs.back().height};
	} else {
		m_storage.data.size = pRaw->size;
		m_storage.raw.bytes.push_back(pRaw->bytes.back());
	}
	m_storage.transfer = load(m_vram, m_storage.data.image, info.format, m_storage.data.size, m_storage.raw.bytes, m_name);
	m_storage.data.format = info.format;
	VRAM& v = m_vram;
	Device& d = v.m_device;
	vk::ImageViewType const type = m_storage.data.type == Type::e3D ? vk::ImageViewType::e3D : vk::ImageViewType::e2D;
	m_storage.data.imageView = d.createImageView(m_storage.data.image->image, m_storage.data.format, vk::ImageAspectFlagBits::eColor, type);
	return true;
}

void Texture::destroy() {
	VRAM& v = m_vram;
	Device& d = v.m_device;
	d.defer([&v, &d, data = m_storage.data, r = m_storage.raw]() mutable {
		v.destroy(data.image);
		d.destroy(data.imageView);
		for (auto const& img : r.imgs) {
			utils::release(img);
		}
	});
	m_storage = {};
}

bool Texture::valid() const {
	return m_storage.data.image.valid();
}

bool Texture::busy() const {
	return le::utils::futureState(m_storage.transfer) == le::FutureState::eDeferred;
}

bool Texture::ready() const {
	return valid() && !busy();
}

void Texture::wait() {
	if (m_storage.transfer.valid()) {
		m_storage.transfer.get();
	}
}

Texture::Data const& Texture::data() const noexcept {
	return m_storage.data;
}
} // namespace le::graphics
