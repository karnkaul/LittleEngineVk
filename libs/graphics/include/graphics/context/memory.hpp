#pragma once
#include <atomic>
#include <unordered_map>
#include <core/log.hpp>
#include <core/monotonic_map.hpp>
#include <core/ref.hpp>
#include <core/view.hpp>
#include <graphics/types.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le::graphics {
class Device;

struct QShare final {
	vk::SharingMode desired;

	// TODO: Use exclusive queues
	constexpr QShare(vk::SharingMode desired = vk::SharingMode::eConcurrent) : desired(desired) {
	}

	vk::SharingMode operator()(Device const& device, QFlags queues) const;
};

struct ResourceCreateInfo {
	std::string name;
	QShare share;
	QFlags queueFlags = QType::eGraphics | QType::eTransfer;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

struct Buffer::CreateInfo : ResourceCreateInfo {
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
};

struct Image::CreateInfo final : ResourceCreateInfo {
	vk::ImageCreateInfo createInfo;
};

class Memory {
  public:
	enum class ResourceType { eBuffer, eImage, eCOUNT_ };

	Memory(Device& device);
	~Memory();

	View<Buffer> construct(Buffer::CreateInfo const& info, bool bSilent = false);
	bool destroy(View<Buffer> buffer, bool bSilent = false);
	[[nodiscard]] bool mapMemory(Buffer& out_buffer) const;
	void unmapMemory(Buffer& out_buffer) const;
	bool write(Buffer& out_buffer, void const* pData, Buffer::Span const& range = {}) const;

	View<Image> construct(Image::CreateInfo const& info);
	bool destroy(View<Image> image);

#if defined(LEVK_DEBUG)
	bool m_bLogAllocs = false;
#else
	bool m_bLogAllocs = false;
#endif

	dl::level m_logLevel = dl::level::debug;
	Ref<Device> m_device;

  private:
	std::string logCount();
	void destroyImpl(Buffer& out_buffer, bool bSilent);
	void destroyImpl(Image& out_image);

  protected:
	VmaAllocator m_allocator;
	EnumArray<ResourceType, std::atomic<u64>> m_allocations;
	kt::lockable<> m_mutex;
	MonotonicMap<Buffer> m_buffers;
	MonotonicMap<Image> m_images;
};
} // namespace le::graphics
