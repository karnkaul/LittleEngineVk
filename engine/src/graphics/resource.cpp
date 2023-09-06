#include <le/graphics/allocator.hpp>
#include <le/graphics/command_buffer.hpp>
#include <le/graphics/device.hpp>
#include <le/graphics/image_barrier.hpp>
#include <le/graphics/resource.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <numeric>

namespace le::graphics {
namespace {
struct VmaImage {
	VmaAllocation allocation{};
	vk::Image image{};
	vk::UniqueImageView image_view{};

	static auto make(ImageCreateInfo const& m_create_info, vk::Extent2D extent, std::uint32_t mip_levels = 1) {
		auto ret = VmaImage{};
		auto vaci = VmaAllocationCreateInfo{};
		vaci.usage = VMA_MEMORY_USAGE_AUTO;
		auto ici = vk::ImageCreateInfo{};
		ici.usage = m_create_info.usage;
		ici.imageType = vk::ImageType::e2D;
		ici.tiling = m_create_info.tiling;
		ici.arrayLayers = m_create_info.view_type == vk::ImageViewType::eCube ? Image::cubemap_layers_v : 1;
		if (ici.arrayLayers == Image::cubemap_layers_v) { ici.flags |= vk::ImageCreateFlagBits::eCubeCompatible; }
		ici.mipLevels = mip_levels;
		ici.extent = vk::Extent3D{extent, 1};
		ici.format = m_create_info.format;
		ici.samples = m_create_info.samples;
		auto const vici = static_cast<VkImageCreateInfo>(ici);

		// NOLINTNEXTLINE
		auto image = VkImage{};
		// NOLINTNEXTLINE
		if (vmaCreateImage(Allocator::self().vma(), &vici, &vaci, &image, &ret.allocation, {}) != VK_SUCCESS) {
			throw Error{"Failed to allocate Vulkan Image"};
		}
		ret.image = image;

		auto const isr = vk::ImageSubresourceRange{m_create_info.aspect, 0, ici.mipLevels, 0, ici.arrayLayers};
		auto ivci = vk::ImageViewCreateInfo{};
		ivci.viewType = m_create_info.view_type;
		ivci.format = m_create_info.format;
		ivci.components.r = ivci.components.g = ivci.components.b = ivci.components.a = vk::ComponentSwizzle::eIdentity;
		ivci.subresourceRange = isr;
		ivci.image = image;
		ret.image_view = Device::self().get_device().createImageViewUnique(ivci);

		return ret;
	}
};

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

struct CopyBufferToImage {
	vk::Image target_image{};
	vk::Extent2D image_extent{};
	vk::Offset2D target_offset{};
	vk::Buffer source_bytes{};
	vk::Extent2D source_extent{};
	std::uint32_t array_layers{1};
	std::uint32_t mip_levels{1};

	auto operator()(vk::CommandBuffer cmd) const {
		auto const isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, array_layers);
		auto const vk_extent = vk::Extent3D{source_extent, 1u};
		auto const bic = vk::BufferImageCopy({}, {}, {}, isrl, vk::Offset3D{target_offset, 0}, vk_extent);
		auto barrier = ImageBarrier{target_image, mip_levels, array_layers};
		barrier.set_full_barrier(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal).transition(cmd);
		cmd.copyBufferToImage(source_bytes, target_image, vk::ImageLayout::eTransferDstOptimal, bic);
		barrier.set_full_barrier(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal).transition(cmd);

		if (mip_levels > 1) { MipMapWriter{barrier, image_extent, cmd, mip_levels, array_layers}(); }
	}
};

struct CopyImage {
	vk::Image image{};
	vk::ImageLayout layout{};
	vk::Extent2D extent{};
	vk::Offset2D offset{};
};

struct CopyImageToImage {
	CopyImage source{};
	CopyImage target{};

	std::uint32_t array_layers{1};
	std::uint32_t mip_levels{1};

	auto operator()(vk::CommandBuffer cmd) const {
		auto const isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, array_layers);
		auto source_barrier = ImageBarrier{source.image, mip_levels, array_layers};
		auto target_barrier = ImageBarrier{target.image, mip_levels, array_layers};
		source_barrier.set_full_barrier(source.layout, vk::ImageLayout::eTransferSrcOptimal).transition(cmd);
		target_barrier.set_full_barrier(target.layout, vk::ImageLayout::eTransferDstOptimal).transition(cmd);
		auto image_copy = vk::ImageCopy{isrl, vk::Offset3D{source.offset, 0}, isrl, vk::Offset3D{target.offset, 0}, vk::Extent3D{source.extent, 1}};
		cmd.copyImage(source.image, vk::ImageLayout::eTransferSrcOptimal, target.image, vk::ImageLayout::eTransferDstOptimal, image_copy);
		source_barrier.set_full_barrier(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal).transition(cmd);
		target_barrier.set_full_barrier(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal).transition(cmd);

		if (mip_levels > 1) { MipMapWriter{target_barrier, target.extent, cmd, mip_levels, array_layers}(); }
	}
};

std::atomic<vk::DeviceSize> g_buffer_bytes{}; // NOLINT
std::atomic<vk::DeviceSize> g_image_bytes{};  // NOLINT
} // namespace

Buffer::Buffer(vk::BufferUsageFlags usage, vk::DeviceSize capacity, bool host_visible) : m_usage(usage), m_host(host_visible) { resize(capacity); }

Buffer::~Buffer() { destroy(); }

auto Buffer::destroy() -> void {
	vmaDestroyBuffer(Allocator::instance(), m_buffer, m_allocation);
	g_buffer_bytes -= m_capacity;
}

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

	destroy();

	m_buffer = buffer;
	m_allocation = allocation;
	m_capacity = bci.size;
	m_mapped = alloc_info.pMappedData;
	m_size = {};

	g_buffer_bytes += m_capacity;

	if (m_host) { assert(m_mapped); }
}

auto Buffer::bytes_allocated() -> vk::DeviceSize { return g_buffer_bytes; }

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
	recreate(extent);
}

Image::~Image() { destroy(); }

auto Image::destroy() -> void {
	vmaDestroyImage(Allocator::instance(), m_image, m_allocation);
	g_image_bytes -= m_bytes_allocated;
}

auto Image::recreate(vk::Extent2D extent) -> void {
	if (extent.width == 0 || extent.height == 0) { return; }

	auto const mip_levels = m_create_info.mip_map ? compute_mip_levels(extent) : 1;
	auto vma_image = VmaImage::make(m_create_info, extent, mip_levels);

	destroy();

	m_image = vma_image.image;
	m_view = std::move(vma_image.image_view);
	m_allocation = vma_image.allocation;
	m_extent = extent;
	m_layout = vk::ImageLayout::eUndefined;
	m_mip_levels = mip_levels;

	auto info = VmaAllocationInfo{};
	vmaGetAllocationInfo(Allocator::self(), m_allocation, &info);
	m_bytes_allocated = info.size;

	g_image_bytes += m_bytes_allocated;
}

auto Image::copy_from(std::span<Layer const> layers, vk::Extent2D target_extent) -> bool {
	auto const array_layers = m_create_info.view_type == vk::ImageViewType::eCube ? cubemap_layers_v : 1;
	if (target_extent.width == 0 || target_extent.height == 0) { return false; }
	if (layers.size() != array_layers) { return false; }

	if (m_extent != target_extent) { recreate(target_extent); }
	auto const accumulate_size = [](std::size_t total, Layer const layer) { return total + layer.size_bytes(); };
	auto const size = std::accumulate(layers.begin(), layers.end(), std::size_t{}, accumulate_size);
	auto staging = HostBuffer{vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, size};

	auto* ptr = static_cast<std::byte*>(staging.mapped());
	for (auto const& image : layers) {
		std::memcpy(ptr, image.data(), image.size_bytes());
		// NOLINTNEXTLINE
		ptr += image.size_bytes();
	}

	auto cmd = CommandBuffer{};
	CopyBufferToImage{
		.target_image = m_image,
		.image_extent = m_extent,
		.target_offset = {},
		.source_bytes = staging.buffer(),
		.source_extent = target_extent,
		.array_layers = array_layers,
		.mip_levels = m_mip_levels,
	}(cmd.get());

	cmd.submit();

	return true;
}

auto Image::overwrite(Bitmap const& bitmap, glm::uvec2 top_left) -> bool {
	if (m_create_info.view_type == vk::ImageViewType::eCube) { return false; }

	auto const overwrite_extent = bitmap.extent + top_left;
	auto const current_extent = glm::uvec2{m_extent.width, m_extent.height};
	if (overwrite_extent.x > current_extent.x || overwrite_extent.y > current_extent.y) { return false; }

	auto staging = HostBuffer{vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, bitmap.bytes.size_bytes()};
	std::memcpy(staging.mapped(), bitmap.bytes.data(), bitmap.bytes.size_bytes());

	auto cmd = CommandBuffer{};
	auto const offset = glm::ivec2{top_left};
	CopyBufferToImage{
		.target_image = m_image,
		.image_extent = m_extent,
		.target_offset = {offset.x, offset.y},
		.source_bytes = staging.buffer(),
		.source_extent = {bitmap.extent.x, bitmap.extent.y},
		.array_layers = 1,
		.mip_levels = m_mip_levels,
	}(cmd.get());

	cmd.submit();

	return true;
}

auto Image::overwrite(std::span<ImageWrite const> writes) -> bool {
	if (m_create_info.view_type == vk::ImageViewType::eCube) { return false; }

	auto const accumulate_size = [](std::size_t total, ImageWrite const& iw) { return total + iw.bitmap.bytes.size_bytes(); };
	auto const size = std::accumulate(writes.begin(), writes.end(), std::size_t{}, accumulate_size);
	auto staging = HostBuffer{vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, size};

	auto const isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto bics = std::vector<vk::BufferImageCopy>{};
	bics.reserve(writes.size());
	auto* ptr = static_cast<std::byte*>(staging.mapped());
	auto buffer_offset = vk::DeviceSize{};
	for (auto const& iw : writes) {
		// NOLINTNEXTLINE
		std::memcpy(ptr + buffer_offset, iw.bitmap.bytes.data(), iw.bitmap.bytes.size_bytes());
		auto const image_offset = glm::ivec2{iw.top_left};
		auto const bic = vk::BufferImageCopy{
			buffer_offset, {}, {}, isrl, vk::Offset3D{image_offset.x, image_offset.y, 0}, {iw.bitmap.extent.x, iw.bitmap.extent.y, 1},
		};
		bics.push_back(bic);
		buffer_offset += iw.bitmap.bytes.size_bytes();
	}

	auto cmd = CommandBuffer{};
	auto barrier = ImageBarrier{m_image, m_mip_levels, 1};
	barrier.set_full_barrier(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal).transition(cmd);
	cmd.get().copyBufferToImage(staging.buffer(), m_image, vk::ImageLayout::eTransferDstOptimal, bics);
	barrier.set_full_barrier(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal).transition(cmd);

	if (m_mip_levels > 1) { MipMapWriter{barrier, m_extent, cmd, m_mip_levels, 1}(); }
	return true;
}

auto Image::resize(vk::Extent2D extent) -> void {
	assert(extent.width < 40960 && extent.height < 40960);
	auto const mip_levels = m_create_info.mip_map ? compute_mip_levels(extent) : 1;
	auto new_image = VmaImage::make(m_create_info, extent, mip_levels);
	auto const copy_src = CopyImage{
		.image = m_image,
		.layout = vk::ImageLayout::eShaderReadOnlyOptimal,
		.extent = m_extent,
	};
	auto const copy_dst = CopyImage{
		.image = new_image.image,
		.layout = vk::ImageLayout::eUndefined,
		.extent = extent,
	};

	auto cmd = CommandBuffer{};
	CopyImageToImage{
		.source = copy_src,
		.target = copy_dst,
		.array_layers = m_create_info.view_type == vk::ImageViewType::eCube ? cubemap_layers_v : 1,
		.mip_levels = mip_levels,
	}(cmd);

	cmd.submit();
}

auto Image::bytes_allocated() -> vk::DeviceSize { return g_image_bytes; }
} // namespace le::graphics
