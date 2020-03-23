#pragma once
#include "common.hpp"

namespace le::vuk
{
constexpr bool g_VRAM_bLogAllocs = true;

struct TransferOp final
{
	vk::Queue queue;
	vk::CommandPool pool;

	vk::Fence transferred;
	vk::CommandBuffer commandBuffer;
};

struct ImageData final
{
	vk::Extent3D size;
	vk::Format format;
	vk::ImageTiling tiling;
	vk::ImageUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	vk::ImageType type = vk::ImageType::e2D;
	QFlags queueFlags = QFlags(QFlag::eGraphics, QFlag::eTransfer);
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

struct BufferData final
{
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

namespace vram
{
inline VmaAllocator g_allocator;

void init(vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice);
void deinit();

Buffer createBuffer(BufferData const& data);
void copy(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, TransferOp* pOp);
bool write(Buffer buffer, void const* pData);

Image createImage(ImageData const& data);

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
} // namespace le::vuk
