#pragma once
#include <future>
#include <utility>
#include <dumb_log/log.hpp>
#include <engine/levk.hpp>
#include <engine/window/common.hpp>
#include <gfx/common.hpp>

namespace le::gfx {
#if defined(LEVK_DEBUG)
inline bool g_VRAM_bLogAllocs = false;
#else
inline constexpr bool g_VRAM_bLogAllocs = false;
#endif

inline dl::level g_VRAM_logLevel = dl::level::debug;

struct QShare final {
	vk::SharingMode desired;

	// TODO: Use exclusive queues
	constexpr QShare(vk::SharingMode desired = vk::SharingMode::eConcurrent) : desired(desired) {
	}

	vk::SharingMode operator()(QFlags queues) const;
};

struct ImageInfo final {
	vk::ImageCreateInfo createInfo;
	QShare share;
	std::string name;
	QFlags queueFlags = QFlags(QFlag::eGraphics) | QFlag::eTransfer;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

struct BufferInfo final {
	std::string name;
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	QShare share;
	QFlags queueFlags = QFlags(QFlag::eGraphics) | QFlag::eTransfer;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

namespace vram {
inline VmaAllocator g_allocator;

void init(View<engine::MemRange> stagingReserve);
void deinit();

void update();

Buffer createBuffer(BufferInfo const& info, bool bSilent = false);
bool write(Buffer& out_dst, void const* pData, vk::DeviceSize size = 0);
[[nodiscard]] bool mapMemory(Buffer& out_buffer);
void unmapMemory(Buffer& out_buffer);
[[nodiscard]] std::future<void> copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size = 0);
[[nodiscard]] std::future<void> stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size = 0);

Image createImage(ImageInfo const& info);
[[nodiscard]] std::future<void> copy(View<View<u8>> pixelsArr, Image const& dst, LayoutTransition layouts);

void release(Buffer buffer, bool bSilent = false);
void release(Image image);

template <typename T1, typename... Tn>
void release(T1 t1, Tn... tn) {
	static_assert(std::is_same_v<T1, Image> || std::is_same_v<T1, Buffer>, "Invalid Type!");
	release(t1);
	release(tn...);
}
} // namespace vram
} // namespace le::gfx
