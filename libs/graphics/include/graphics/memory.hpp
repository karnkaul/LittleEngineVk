#pragma once
#include <vk_mem_alloc.h>
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/image_ref.hpp>
#include <graphics/qtype.hpp>
#include <ktl/either.hpp>
#include <atomic>
#include <optional>

namespace le::graphics {
class Device;
class Queues;

enum class BlitFilter { eLinear, eNearest };

class Memory : public Pinned {
  public:
	enum class Type { eBuffer, eImage };

	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;

	struct AllocInfo;
	struct Resource;
	struct Deleter;

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

	static vk::SharingMode sharingMode(Queues const& queues, QCaps const caps);
	static void copy(vk::CommandBuffer cb, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
	static void copy(vk::CommandBuffer cb, vk::Buffer src, vk::Image dst, vAP<vk::BufferImageCopy> regions, ImgMeta const& meta);
	static void copy(vk::CommandBuffer cb, TPair<vk::Image> images, vk::Extent3D extent, vk::ImageAspectFlags aspects);
	static void blit(vk::CommandBuffer cb, TPair<vk::Image> images, TPair<vk::Extent3D> extents, TPair<vk::ImageAspectFlags> aspects, BlitFilter filter);
	static void imageBarrier(vk::CommandBuffer cb, vk::Image image, ImgMeta const& meta);
	static vk::BufferImageCopy bufferImageCopy(vk::Extent3D extent, vk::ImageAspectFlags aspects, vk::DeviceSize offset, u32 layerIdx);
	static vk::ImageBlit imageBlit(TPair<Memory::ImgMeta> const& meta, TPair<vk::Offset3D> const& srcOff, TPair<vk::Offset3D> const& dstOff) noexcept;

	std::optional<Resource> makeBuffer(AllocInfo const& ai, vk::BufferCreateInfo const& bci) const;
	std::optional<Resource> makeImage(AllocInfo const& ai, vk::ImageCreateInfo const& ici) const;
	void defer(Resource const& resource) const;
	void* map(Resource& out_resource) const;
	void unmap(Resource& out_resource) const;

	u64 bytes(Type type) const noexcept { return m_allocations[type].load(); }

	not_null<Device*> m_device;

  protected:
	VmaAllocator m_allocator;
	mutable EnumArray<Type, std::atomic<u64>, 2> m_allocations;
};

struct Memory::AllocInfo {
	QCaps qcaps = QType::eGraphics;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	vk::MemoryPropertyFlags preferred;
};

struct Memory::Resource {
	using MPFlags = vk::MemoryPropertyFlags;
	struct {
		vk::DeviceMemory memory;
		vk::DeviceSize offset{};
		vk::DeviceSize size{};
	} alloc;
	ktl::either<vk::Buffer, vk::Image> resource;
	VmaAllocator allocator{};
	VmaAllocation handle{};
	vk::SharingMode mode{};
	vk::DeviceSize size{};
	void* data{};
	QCaps qcaps;
};

struct Memory::Deleter {
	void operator()(not_null<Memory const*> memory, Resource const& resource) const;
};
} // namespace le::graphics
