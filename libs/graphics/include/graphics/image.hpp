#pragma once
#include <graphics/memory.hpp>
#include <graphics/utils/deferred.hpp>

namespace le::graphics {
class Image {
  public:
	struct CreateInfo;

	static constexpr auto allocation_type_v = Memory::Type::eImage;

	static constexpr vk::Format srgb_v = vk::Format::eR8G8B8A8Srgb;
	static constexpr vk::Format linear_v = vk::Format::eR8G8B8A8Unorm;

	static CreateInfo info(Extent2D extent, vk::ImageUsageFlags usage, vk::ImageAspectFlags view, VmaMemoryUsage vmaUsage, vk::Format format) noexcept;
	static CreateInfo textureInfo(Extent2D extent, vk::Format format = srgb_v, bool mips = true) noexcept;
	static CreateInfo cubemapInfo(Extent2D extent, vk::Format format = srgb_v) noexcept;
	static CreateInfo storageInfo(Extent2D extent, vk::Format format = linear_v) noexcept;
	static u32 mipLevels(Extent2D extent) noexcept;

	Image(not_null<Memory*> memory) noexcept;
	Image(not_null<Memory*> memory, CreateInfo const& info);
	Image(Image&& rhs) noexcept : Image(rhs.m_memory) { exchg(*this, rhs); }
	Image& operator=(Image rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Image();

	ImageRef ref() const noexcept;
	vk::Image image() const noexcept { return m_image.resource.get<vk::Image>(); }
	vk::ImageView view() const noexcept { return m_data.view; }
	vk::ImageViewType viewType() const noexcept { return m_data.viewType; }
	vk::Format format() const noexcept { return m_data.format; }
	u32 layerCount() const noexcept { return m_data.layerCount; }
	u32 mipCount() const noexcept { return m_data.mipCount; }
	BlitFlags blitFlags() const noexcept { return m_data.blitFlags; }
	vk::Extent3D extent() const noexcept { return m_data.extent; }
	Extent2D extent2D() const noexcept { return cast(extent()); }
	vk::ImageUsageFlags usage() const noexcept { return m_data.usage; }

	void const* mapped() const noexcept { return m_image.data; }
	void const* map();
	bool unmap();

  private:
	static void exchg(Image& lhs, Image& rhs) noexcept;

  protected:
	struct Data {
		Deferred<vk::ImageView> view;
		vk::ImageViewType viewType{};
		vk::Extent3D extent = {};
		vk::ImageTiling tiling{};
		vk::ImageUsageFlags usage;
		VmaMemoryUsage vmaUsage{};
		vk::Format format{};
		u32 layerCount = 1U;
		u32 mipCount = 1U;
		BlitFlags blitFlags;
	};
	Memory::Resource m_image;
	Data m_data;
	not_null<Memory*> m_memory;

	friend class VRAM;
};

struct Image::CreateInfo final : Memory::AllocInfo {
	vk::ImageCreateInfo createInfo;
	struct {
		vk::Format format{};
		vk::ImageAspectFlags aspects;
		vk::ImageViewType type = vk::ImageViewType::e2D;
	} view;
	bool mipMaps = false;
};
} // namespace le::graphics
