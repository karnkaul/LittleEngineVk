#pragma once
#include "common.hpp"

namespace le::gfx
{
constexpr bool g_VRAM_bLogAllocs = true;

struct ImageInfo final
{
	vk::Extent3D size;
	vk::Format format;
	vk::ImageTiling tiling;
	vk::ImageUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	vk::ImageType type = vk::ImageType::e2D;
	QFlags queueFlags = QFlags({QFlag::eGraphics, QFlag::eTransfer});
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

struct BufferInfo final
{
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	QFlags queueFlags = QFlags({QFlag::eGraphics, QFlag::eTransfer});
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

namespace vram
{
inline VmaAllocator g_allocator;

void init();
void deinit();

Buffer createBuffer(BufferInfo const& info);
bool write(Buffer buffer, void const* pData, vk::DeviceSize size = 0);
vk::Fence copy(Buffer const& src, Buffer const& dst, vk::DeviceSize size = 0);
vk::Fence stage(Buffer const& deviceBuffer, void const* pData, vk::DeviceSize size = 0);

Image createImage(ImageInfo const& info);

void release(Buffer buffer);
void release(Image image);

template <typename T1, typename... Tn>
void release(T1 t1, Tn... tn)
{
	static_assert(std::is_same_v<T1, Image> || std::is_same_v<T1, Buffer>, "Invalid Type!");
	release(t1);
	release(tn...);
}
} // namespace vram
} // namespace le::gfx
