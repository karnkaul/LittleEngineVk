#include <levk/core/log_channel.hpp>
#include <levk/graphics/image.hpp>
#include <levk/graphics/texture.hpp>

namespace le::graphics {
namespace {
constexpr bool hostVisible(VmaMemoryUsage usage) noexcept {
	return usage == VMA_MEMORY_USAGE_CPU_TO_GPU || usage == VMA_MEMORY_USAGE_GPU_TO_CPU || usage == VMA_MEMORY_USAGE_CPU_ONLY ||
		   usage == VMA_MEMORY_USAGE_CPU_COPY;
}

constexpr bool canMip(BlitCaps bc, vk::ImageTiling tiling) noexcept {
	if (tiling == vk::ImageTiling::eLinear) {
		return bc.linear.all(BlitFlags(BlitFlag::eLinearFilter, BlitFlag::eSrc, BlitFlag::eDst));
	} else {
		return bc.optimal.all(BlitFlags(BlitFlag::eLinearFilter, BlitFlag::eSrc, BlitFlag::eDst));
	}
}
} // namespace

Image::CreateInfo Image::info(Extent2D extent, vk::ImageUsageFlags usage, vk::ImageAspectFlags view, VmaMemoryUsage vmaUsage, vk::Format format) noexcept {
	CreateInfo ret;
	ret.createInfo.extent = vk::Extent3D(extent.x, extent.y, 1);
	ret.createInfo.usage = usage;
	ret.vmaUsage = vmaUsage;
	bool const linear = vmaUsage != VMA_MEMORY_USAGE_UNKNOWN && vmaUsage != VMA_MEMORY_USAGE_GPU_ONLY && vmaUsage != VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
	ret.view.aspects = view;
	ret.qcaps = QType::eGraphics;
	if (view != vk::ImageAspectFlags()) {
		ret.view.format = format;
		ret.view.type = vk::ImageViewType::e2D;
	}
	ret.createInfo.format = format;
	ret.createInfo.tiling = linear ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
	ret.createInfo.imageType = vk::ImageType::e2D;
	ret.createInfo.mipLevels = 1U;
	ret.createInfo.arrayLayers = 1U;
	return ret;
}

Image::CreateInfo Image::textureInfo(Extent2D extent, vk::Format format, bool mips) noexcept {
	auto ret = info(extent, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format);
	ret.mipMaps = mips;
	return ret;
}

Image::CreateInfo Image::cubemapInfo(Extent2D extent, vk::Format format) noexcept {
	auto ret = info(extent, Texture::usage_v, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, format);
	ret.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	ret.createInfo.arrayLayers = 6U;
	ret.view.type = vk::ImageViewType::eCube;
	return ret;
}

Image::CreateInfo Image::storageInfo(Extent2D extent, vk::Format format) noexcept {
	auto ret = info(extent, vIUFB::eTransferDst, vk::ImageAspectFlags(), VMA_MEMORY_USAGE_GPU_TO_CPU, format);
	ret.createInfo.tiling = vk::ImageTiling::eLinear;
	return ret;
}

u32 Image::mipLevels(Extent2D extent) noexcept { return static_cast<u32>(std::floor(std::log2(std::max(extent.x, extent.y)))) + 1U; }

ImageRef Image::ref() const noexcept {
	bool const linear = m_data.tiling == vk::ImageTiling::eLinear;
	return ImageRef{ImageView{image(), m_view, extent2D(), m_data.format}, linear};
}

LayerMip Image::layerMip() const noexcept {
	LayerMip ret;
	ret.layer = {0U, layerCount()};
	ret.mip = {0U, mipCount()};
	return ret;
}

Image::Image(not_null<Memory*> memory, CreateInfo const& info) : m_memory(memory) {
	Device& d = *memory->m_device;
	u32 const family = d.queues().primary().family();
	vk::ImageCreateInfo imageInfo = info.createInfo;
	imageInfo.sharingMode = Memory::sharingMode(d.queues(), info.qcaps);
	imageInfo.queueFamilyIndexCount = 1U;
	imageInfo.pQueueFamilyIndices = &family;
	auto const blitCaps = memory->m_device->physicalDevice().blitCaps(imageInfo.format);
	imageInfo.mipLevels = info.mipMaps && canMip(blitCaps, imageInfo.tiling) ? mipLevels(cast(imageInfo.extent)) : 1U;
	if (auto img = m_memory->makeImage(info, imageInfo)) {
		m_data.extent = imageInfo.extent;
		m_data.usage = imageInfo.usage;
		m_data.format = imageInfo.format;
		m_data.vmaUsage = info.vmaUsage;
		m_data.mipCount = imageInfo.mipLevels;
		m_data.tiling = imageInfo.tiling;
		m_data.layerCount = imageInfo.arrayLayers;
		m_data.blitFlags = imageInfo.tiling == vk::ImageTiling::eLinear ? blitCaps.linear : blitCaps.optimal;
		m_image = {*img, m_memory->m_device, m_memory};
		if (info.view.aspects != vk::ImageAspectFlags() && info.view.format != vk::Format()) {
			m_view = m_view.make(d.makeImageView(image(), info.view.format, info.view.aspects, info.view.type, imageInfo.mipLevels), &d);
			m_data.viewType = info.view.type;
		}
	} else {
		logE(LC_LibUser, "[{}] Failed to create Image!", g_name);
		EXPECT(false);
	}
}

void const* Image::map() {
	if (!hostVisible(m_data.vmaUsage)) {
		logE(LC_LibUser, "[{}] Attempt to map GPU-only Buffer!", g_name);
		return nullptr;
	}
	if (!m_image.get().data) { m_memory->map(m_image.get()); }
	return mapped();
}

bool Image::unmap() {
	if (m_image.get().data) {
		m_memory->unmap(m_image.get());
		return true;
	}
	return false;
}
} // namespace le::graphics
