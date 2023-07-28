#include <spaced/graphics/allocator.hpp>
#include <spaced/graphics/command_buffer.hpp>
#include <spaced/graphics/device.hpp>
#include <spaced/graphics/image_barrier.hpp>
#include <spaced/graphics/resource.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace spaced::graphics {
namespace {

struct MipMapWriter {
	// NOLINTNEXTLINE
	ImageBarrier& ib;

	vk::Extent2D extent;
	vk::CommandBuffer cb;
	std::uint32_t mip_levels;

	std::uint32_t layer_count{1};

	auto blit_mips(std::uint32_t const src_level, vk::Offset3D const src_offset, vk::Offset3D const dst_offset) const -> void {
		auto const src_isrl = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, src_level, 0, layer_count};
		auto const dst_isrl = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, src_level + 1, 0, layer_count};
		auto const region = vk::ImageBlit{src_isrl, {vk::Offset3D{}, src_offset}, dst_isrl, {vk::Offset3D{}, dst_offset}};
		cb.blitImage(ib.barrier.image, vk::ImageLayout::eTransferSrcOptimal, ib.barrier.image, vk::ImageLayout::eTransferDstOptimal, region,
					 vk::Filter::eLinear);
	}

	auto blit_next_mip(std::uint32_t const src_level, vk::Offset3D const src_offset, vk::Offset3D const dst_offset) -> void {
		ib.barrier.oldLayout = vk::ImageLayout::eUndefined;
		ib.barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		ib.barrier.subresourceRange.baseMipLevel = src_level + 1;
		ib.transition(cb);

		blit_mips(src_level, src_offset, dst_offset);

		ib.barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		ib.barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		ib.transition(cb);
	}

	auto operator()() -> void {
		ib.barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		ib.barrier.subresourceRange.baseArrayLayer = 0;
		ib.barrier.subresourceRange.layerCount = layer_count;
		ib.barrier.subresourceRange.baseMipLevel = 0;
		ib.barrier.subresourceRange.levelCount = 1;

		ib.barrier.srcAccessMask = ib.barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
		ib.barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
		ib.barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;

		ib.barrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ib.barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		ib.barrier.subresourceRange.baseMipLevel = 0;
		ib.transition(cb);

		ib.barrier.srcStageMask = ib.barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;

		auto src_extent = vk::Extent3D{extent, 1};
		for (std::uint32_t mip = 0; mip + 1 < mip_levels; ++mip) {
			vk::Extent3D dst_extent = vk::Extent3D(std::max(src_extent.width / 2, 1u), std::max(src_extent.height / 2, 1u), 1u);
			auto const src_offset = vk::Offset3D{static_cast<int>(src_extent.width), static_cast<int>(src_extent.height), 1};
			auto const dst_offset = vk::Offset3D{static_cast<int>(dst_extent.width), static_cast<int>(dst_extent.height), 1};
			blit_next_mip(mip, src_offset, dst_offset);
			src_extent = dst_extent;
		}

		ib.barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		ib.barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ib.barrier.subresourceRange.baseMipLevel = 0;
		ib.barrier.subresourceRange.levelCount = mip_levels;
		ib.barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
		ib.transition(cb);
	}
};
} // namespace

Buffer::Buffer(vk::BufferUsageFlags usage, vk::DeviceSize capacity, bool host_visible) : m_usage(usage), m_host(host_visible) { resize(capacity); }

Buffer::~Buffer() { vmaDestroyBuffer(Allocator::instance(), m_buffer, m_allocation); }

auto Buffer::resize(std::size_t new_capacity) -> void {
	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = VMA_MEMORY_USAGE_AUTO;
	if (m_host) { vaci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT; }
	auto const bci = vk::BufferCreateInfo{{}, new_capacity, m_usage | vk::BufferUsageFlagBits::eTransferDst};
	auto vbci = static_cast<VkBufferCreateInfo>(bci);

	// NOLINTNEXTLINE
	auto allocation = VmaAllocation{};
	// NOLINTNEXTLINE
	auto buffer = VkBuffer{};
	auto alloc_info = VmaAllocationInfo{};
	if (vmaCreateBuffer(Allocator::instance(), &vbci, &vaci, &buffer, &allocation, &alloc_info) != VK_SUCCESS) {
		throw Error{"Failed to allocate Vulkan Buffer"};
	}

	if (m_buffer) { vmaDestroyBuffer(Allocator::instance(), m_buffer, m_allocation); }

	m_buffer = buffer;
	m_allocation = allocation;
	m_capacity = bci.size;
	m_mapped = alloc_info.pMappedData;
	m_size = {};

	if (m_host) { assert(m_mapped); }
}

auto HostBuffer::write(void const* data, std::size_t size) -> void {
	if (size > m_capacity) { resize(size); }
	if (size > 0) { std::memcpy(m_mapped, data, size); }
	m_size = size;
}

auto DeviceBuffer::write(void const* data, std::size_t size) -> void {
	auto scratch_buffer = std::make_unique<HostBuffer>(vk::BufferUsageFlagBits::eTransferSrc, size);
	scratch_buffer->write(data, size);
	if (size > m_capacity) { resize(size); }
	auto cmd = CommandBuffer{};
	auto const bcr = vk::BufferCopy{{}, {}, size};
	cmd.get().copyBuffer(scratch_buffer->buffer(), m_buffer, bcr);
	m_size = size;
	cmd.submit();
}

auto Image::compute_mip_levels(vk::Extent2D extent) -> std::uint32_t {
	return static_cast<std::uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1u;
}

Image::Image(ImageCreateInfo const& info, vk::Extent2D extent) {
	if (extent == vk::Extent2D{}) { extent = min_extent_v; }
	m_create_info = info;
	resize(extent);
}

Image::~Image() { vmaDestroyImage(Allocator::instance(), m_image, m_allocation); }

auto Image::resize(vk::Extent2D extent) -> void {
	if (extent.width == 0 || extent.height == 0) { return; }

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = VMA_MEMORY_USAGE_AUTO;
	auto ici = vk::ImageCreateInfo{};
	ici.usage = m_create_info.usage;
	ici.imageType = vk::ImageType::e2D;
	ici.tiling = m_create_info.tiling;
	ici.arrayLayers = m_create_info.view_type == vk::ImageViewType::eCube ? cubemap_layers_v : 1;
	if (ici.arrayLayers == cubemap_layers_v) { ici.flags |= vk::ImageCreateFlagBits::eCubeCompatible; }
	ici.mipLevels = compute_mip_levels(extent);
	ici.extent = vk::Extent3D{extent, 1};
	ici.format = m_create_info.format;
	ici.samples = m_create_info.samples;
	auto const vici = static_cast<VkImageCreateInfo>(ici);

	// NOLINTNEXTLINE
	auto image = VkImage{};
	// NOLINTNEXTLINE
	auto allocation = VmaAllocation{};
	if (vmaCreateImage(Allocator::self().vma(), &vici, &vaci, &image, &allocation, {}) != VK_SUCCESS) { throw Error{"Failed to allocate Vulkan Image"}; }

	auto const isr = vk::ImageSubresourceRange{m_create_info.aspect, 0, ici.mipLevels, 0, ici.arrayLayers};
	auto ivci = vk::ImageViewCreateInfo{};
	ivci.viewType = m_create_info.view_type;
	ivci.format = m_create_info.format;
	ivci.components.r = ivci.components.g = ivci.components.b = ivci.components.a = vk::ComponentSwizzle::eIdentity;
	ivci.subresourceRange = isr;
	ivci.image = image;
	m_view = Device::self().device().createImageViewUnique(ivci);

	if (m_image) { vmaDestroyImage(Allocator::instance(), m_image, m_allocation); }

	m_image = image;
	m_allocation = allocation;
	m_extent = extent;
	m_layout = ici.initialLayout;
	m_mip_levels = ici.mipLevels;
}

auto Image::copy_from(std::span<Layer const> layers, vk::Extent2D target_extent, vk::Offset2D offset) -> bool {
	auto const array_layers = m_create_info.view_type == vk::ImageViewType::eCube ? cubemap_layers_v : 1;
	if (target_extent.width == 0 || target_extent.height == 0) { return false; }
	if (layers.size() != array_layers) { return false; }

	if (m_extent != target_extent) { resize(target_extent); }
	auto const accumulate_size = [](std::size_t total, Layer const layer) { return total + layer.size_bytes(); };
	auto const size = std::accumulate(layers.begin(), layers.end(), std::size_t{}, accumulate_size);
	auto staging = HostBuffer{vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, size};

	auto* ptr = static_cast<std::byte*>(staging.mapped());
	for (auto const& image : layers) {
		std::memcpy(ptr, image.data(), image.size_bytes());
		// NOLINTNEXTLINE
		ptr += image.size_bytes();
	}

	auto const vk_extent = vk::Extent3D{target_extent, 1u};
	auto const vk_offset = vk::Offset3D{offset.x, offset.y, 0};
	auto const isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, array_layers);
	auto const bic = vk::BufferImageCopy({}, {}, {}, isrl, vk_offset, vk_extent);
	auto cmd = CommandBuffer{};
	auto barrier = ImageBarrier{*this};
	barrier.set_full_barrier(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal).transition(cmd);
	cmd.get().copyBufferToImage(staging.buffer(), image(), vk::ImageLayout::eTransferDstOptimal, bic);
	barrier.set_full_barrier(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal).transition(cmd);

	if (mip_levels() > 1) { MipMapWriter{barrier, m_extent, cmd, mip_levels(), array_layers}(); }

	cmd.submit();

	return true;
}
} // namespace spaced::graphics
