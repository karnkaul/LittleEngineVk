#pragma once
#include <future>
#include <utility>
#include <core/log_config.hpp>
#include <core/utils.hpp>
#include <engine/levk.hpp>
#include <engine/window/common.hpp>
#include <gfx/common.hpp>

namespace le::gfx
{
#if defined(LEVK_DEBUG)
inline bool g_VRAM_bLogAllocs = false;
#else
constexpr bool g_VRAM_bLogAllocs = false;
#endif

inline io::Level g_VRAM_logLevel = io::Level::eDebug;

struct ImageInfo final
{
	vk::ImageCreateInfo createInfo;
	std::string name;
	QFlags queueFlags = QFlags({QFlag::eGraphics, QFlag::eTransfer});
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

struct BufferInfo final
{
	std::string name;
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	QFlags queueFlags = QFlags({QFlag::eGraphics, QFlag::eTransfer});
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

namespace vram
{
inline VmaAllocator g_allocator;

void init(Span<engine::MemRange> stagingReserve);
void deinit();

void update();

Buffer createBuffer(BufferInfo const& info, bool bSilent = false);
bool write(Buffer& out_dst, void const* pData, vk::DeviceSize size = 0);
[[nodiscard]] bool mapMemory(Buffer& out_buffer);
void unmapMemory(Buffer& out_buffer);
[[nodiscard]] std::future<void> copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size = 0);
[[nodiscard]] std::future<void> stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size = 0);

Image createImage(ImageInfo const& info);
[[nodiscard]] std::future<void> copy(Span<Span<u8>> pixelsArr, Image const& dst, LayoutTransition layouts);

void release(Buffer buffer, bool bSilent = false);
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
