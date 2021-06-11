#pragma once
#include <atomic>
#include <vk_mem_alloc.h>
#include <core/log.hpp>
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <graphics/qflags.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Device;

template <typename T, typename U = T>
using TPair = std::pair<T, U>;

using StagePair = TPair<vk::PipelineStageFlags>;
using AccessPair = TPair<vk::AccessFlags>;
using LayoutPair = TPair<vk::ImageLayout>;

struct Alloc final {
	vk::DeviceMemory memory;
	vk::DeviceSize offset = {};
	vk::DeviceSize actualSize = {};
};

struct QShare final {
	vk::SharingMode desired;

	// TODO: Use exclusive queues
	constexpr QShare(vk::SharingMode desired = vk::SharingMode::eConcurrent) : desired(desired) {}

	vk::SharingMode operator()(Device const& device, QFlags queues) const;
};

class Memory;
class Resource {
  public:
	enum class Type { eBuffer, eImage, eCOUNT_ };

	struct Data {
		Alloc alloc;
		VmaAllocation handle;
		QFlags queueFlags;
		vk::SharingMode mode;
	};

	Resource(not_null<Memory*> memory) : m_memory(memory) {}
	Resource(Resource&&) = default;
	Resource& operator=(Resource&&) = default;
	virtual ~Resource() = default;

	Data const& data() const noexcept { return m_data; }

  private:
	void destroy();

  protected:
	Data m_data;
	not_null<Memory*> m_memory;

	friend class VRAM;
};

class Memory {
  public:
	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;

	struct ImgMeta {
		AccessPair access;
		StagePair stages;
		LayoutPair layouts;
		vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;
		u32 firstLayer = 0;
		u32 layerCount = 1;
		u32 firstMip = 0;
		u32 mipLevels = 1;
	};

	Memory(not_null<Device*> device);
	~Memory();

	u64 bytes(Resource::Type type) const noexcept;

	static void copy(vk::CommandBuffer cb, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
	static void copy(vk::CommandBuffer cb, vk::Buffer src, vk::Image dst, vAP<vk::BufferImageCopy> regions, ImgMeta const& meta);
	static void blit(vk::CommandBuffer cb, vk::Image src, vk::Image dst, TPair<vk::Extent3D> extents, LayoutPair layouts,
					 vk::Filter filter = vk::Filter::eLinear,
					 TPair<vk::ImageAspectFlags> aspects = {vk::ImageAspectFlagBits::eColor, vk::ImageAspectFlagBits::eColor});
	static void imageBarrier(vk::CommandBuffer cb, vk::Image image, ImgMeta const& meta);
	static vk::BufferImageCopy bufferImageCopy(vk::Extent3D extent, vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor, vk::DeviceSize offset = 0,
											   u32 layerIdx = 0, u32 layerCount = 1);

	dl::level m_logLevel = dl::level::debug;
	not_null<Device*> m_device;

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

	Buffer(not_null<Memory*> memory, CreateInfo const& info);
	Buffer(Buffer&&);
	Buffer& operator=(Buffer&&);
	~Buffer() override;

	vk::Buffer buffer() const noexcept { return m_storage.buffer; }
	vk::DeviceSize writeSize() const noexcept { return m_storage.writeSize; }
	std::size_t writeCount() const noexcept { return m_storage.writeCount; }
	vk::BufferUsageFlags usage() const noexcept { return m_storage.usage; }
	Type bufferType() const noexcept { return m_storage.type; }
	void const* mapped() const noexcept { return m_storage.pMap; }

	void const* map();
	bool unmap();
	bool write(void const* pData, vk::DeviceSize size = 0, vk::DeviceSize offset = 0);
	template <typename T>
	bool writeT(T const& t, vk::DeviceSize offset = 0);

  private:
	void destroy();

  protected:
	struct Storage {
		vk::Buffer buffer;
		vk::DeviceSize writeSize = {};
		std::size_t writeCount = 0;
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

	Image(not_null<Memory*> memory, CreateInfo const& info);
	Image(Image&&);
	Image& operator=(Image&&);
	~Image() override;

	vk::Image image() const noexcept { return m_storage.image; }
	vk::ImageView view() const noexcept { return m_storage.view; }
	u32 layerCount() const noexcept { return m_storage.layerCount; }
	vk::Extent3D extent() const noexcept { return m_storage.extent; }
	vk::ImageLayout layout() const noexcept { return m_storage.layout; }
	void layout(vk::ImageLayout layout) noexcept { m_storage.layout = layout; }
	vk::ImageUsageFlags usage() const noexcept { return m_storage.usage; }

  private:
	void destroy();

  protected:
	struct Storage {
		vk::Image image;
		vk::ImageView view;
		vk::DeviceSize allocatedSize = {};
		vk::Extent3D extent = {};
		vk::ImageUsageFlags usage;
		vk::ImageLayout layout;
		u32 layerCount = 1;
	};
	Storage m_storage;

	friend class VRAM;
};

struct ResourceCreateInfo {
	QShare share;
	QFlags queueFlags = QFlags(QType::eGraphics) | QType::eTransfer;
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
	struct {
		vk::Format format;
		vk::ImageAspectFlags aspects;
		vk::ImageViewType type = vk::ImageViewType::e2D;
	} view;
};

// impl

inline u64 Memory::bytes(Resource::Type type) const noexcept { return m_allocations[type].load(); }

template <typename T>
bool Buffer::writeT(T const& t, vk::DeviceSize offset) {
	ENSURE(sizeof(T) <= m_storage.writeSize, "T larger than Buffer size");
	return write(&t, sizeof(T), offset);
}
} // namespace le::graphics
