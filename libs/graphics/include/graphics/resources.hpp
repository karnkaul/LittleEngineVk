#pragma once
#include <atomic>
#include <vk_mem_alloc.h>
#include <core/log.hpp>
#include <core/ref.hpp>
#include <core/std_types.hpp>
#include <graphics/qflags.hpp>
#include <vulkan/vulkan.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_VKRESOURCE_NAMES)
#define LEVK_VKRESOURCE_NAMES
#endif
#endif

namespace le::graphics {
class Device;

using LayoutPair = std::pair<vk::ImageLayout, vk::ImageLayout>;

struct Alloc final {
	vk::DeviceMemory memory;
	vk::DeviceSize offset = {};
	vk::DeviceSize actualSize = {};
};

struct QShare final {
	vk::SharingMode desired;

	// TODO: Use exclusive queues
	constexpr QShare(vk::SharingMode desired = vk::SharingMode::eConcurrent) : desired(desired) {
	}

	vk::SharingMode operator()(Device const& device, QFlags queues) const;
};

class Memory;
class Resource {
  public:
	enum class Type { eBuffer, eImage, eCOUNT_ };

	struct Data {
#if defined(LEVK_VKRESOURCE_NAMES)
		std::string name;
#endif
		Alloc alloc;
		VmaAllocation handle;
		QFlags queueFlags;
		vk::SharingMode mode;
	};

	Resource(Memory& memory) : m_memory(memory) {
	}
	Resource(Resource&&) = default;
	Resource& operator=(Resource&&) = default;
	virtual ~Resource() = default;

	Data const& data() const noexcept {
		return m_data;
	}

  private:
	void destroy();

  protected:
	Data m_data;
	Ref<Memory> m_memory;

	friend class VRAM;
};

class Memory {
  public:
	Memory(Device& device);
	~Memory();

	dl::level m_logLevel = dl::level::debug;
	bool m_bLogAllocs = false;
	Ref<Device> m_device;

	u64 bytes(Resource::Type type) const;
	std::string countsText() const;
	void logCounts(dl::level level) const;

  protected:
	VmaAllocator m_allocator;
	mutable EnumArray<Resource::Type, std::atomic<u64>> m_allocations;

	friend class Buffer;
	friend class Image;
};

class Buffer : public Resource {
  public:
	enum class Type { eCpuToGpu, eGpuOnly };
	struct CreateInfo;
	struct Span;

	static constexpr Resource::Type type = Resource::Type::eBuffer;

	Buffer(Memory& memory, CreateInfo const& info);
	Buffer(Buffer&&);
	Buffer& operator=(Buffer&&);
	~Buffer() override;

	vk::Buffer buffer() const {
		return m_storage.buffer;
	}
	vk::DeviceSize writeSize() const {
		return m_storage.writeSize;
	}
	vk::BufferUsageFlags usage() const {
		return m_storage.usage;
	}
	Type bufferType() const {
		return m_storage.type;
	}
	void const* mapped() const {
		return m_storage.pMap;
	}

	void const* map();
	bool unmap();
	bool write(void const* pData, vk::DeviceSize size = 0, vk::DeviceSize offset = 0);

  private:
	void destroy();

  protected:
	struct Storage {
		vk::Buffer buffer;
		vk::DeviceSize writeSize = {};
		vk::BufferUsageFlags usage;
		Type type;
		void* pMap = nullptr;
	};
	Storage m_storage;

	friend class VRAM;
};

struct Buffer::Span {
	std::size_t offset = 0;
	std::size_t size = 0;
};

class Image : public Resource {
  public:
	struct CreateInfo;

	static constexpr Resource::Type type = Resource::Type::eImage;

	Image(Memory& memory, CreateInfo const& info);
	Image(Image&&);
	Image& operator=(Image&&);
	~Image() override;

	vk::Image image() const noexcept {
		return m_storage.image;
	}
	u32 layerCount() const noexcept {
		return m_storage.layerCount;
	}

  private:
	void destroy();

  protected:
	struct Storage {
		vk::Image image;
		vk::DeviceSize allocatedSize = {};
		vk::Extent3D extent = {};
		u32 layerCount = 1;
	};
	Storage m_storage;

	friend class VRAM;
};

struct ResourceCreateInfo {
	std::string name;
	QShare share;
	QFlags queueFlags = QType::eGraphics | QType::eTransfer;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	vk::MemoryPropertyFlags preferred;
};

struct Buffer::CreateInfo : ResourceCreateInfo {
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
};

struct Image::CreateInfo final : ResourceCreateInfo {
	vk::ImageCreateInfo createInfo;
};
} // namespace le::graphics
