#pragma once
#include <vk_mem_alloc.h>
#include <levk/command_buffer.hpp>
#include <levk/core/error.hpp>
#include <levk/core/is_positive.hpp>
#include <levk/core/thread_index.hpp>
#include <levk/render_device.hpp>
#include <levk/render_resource.hpp>
#include <levk/util.hpp>
#include <numeric>

namespace levk {
struct MakeImageView {
	vk::Image image;
	vk::Format format;

	vk::ImageSubresourceRange subresource{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	vk::ImageViewType type{vk::ImageViewType::e2D};

	[[nodiscard]] auto operator()(vk::Device device) const -> vk::UniqueImageView {
		auto ivci = vk::ImageViewCreateInfo{};
		ivci.viewType = type;
		ivci.format = format;
		ivci.subresourceRange = subresource;
		ivci.image = image;
		return device.createImageViewUnique(ivci);
	}
};

struct MakeMipMaps {
	// NOLINTNEXTLINE
	IRenderImage& out;

	vk::CommandBuffer command_buffer;

	vk::ImageMemoryBarrier2 barrier{};
	std::uint32_t layer_count{1};

	auto blit_mips(std::uint32_t const src_level, vk::Offset3D const src_offset, vk::Offset3D const dst_offset) const -> void {
		auto const src_isrl = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, src_level, 0, layer_count};
		auto const dst_isrl = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, src_level + 1, 0, layer_count};
		auto const region = vk::ImageBlit{src_isrl, {vk::Offset3D{}, src_offset}, dst_isrl, {vk::Offset3D{}, dst_offset}};
		command_buffer.blitImage(barrier.image, vk::ImageLayout::eTransferSrcOptimal, barrier.image, vk::ImageLayout::eTransferDstOptimal, region,
								 vk::Filter::eLinear);
	}

	auto blit_next_mip(std::uint32_t const src_level, vk::Offset3D const src_offset, vk::Offset3D const dst_offset) -> void {
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.subresourceRange.baseMipLevel = src_level + 1;
		record_barriers(command_buffer, {&barrier, 1});

		blit_mips(src_level, src_offset, dst_offset);

		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		record_barriers(command_buffer, {&barrier, 1});
	}

	auto operator()(std::uint32_t const queue_family) -> void {
		layer_count = out.get_image_info().type == vk::ImageViewType::eCube ? 6 : 1;

		barrier.image = out.get_image_info().image;
		barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = queue_family;
		barrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, layer_count};

		barrier.srcAccessMask = barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite;
		barrier.srcStageMask = barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;

		barrier.oldLayout = out.get_image_info().layout;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.subresourceRange.baseMipLevel = 0;
		record_barriers(command_buffer, {&barrier, 1});

		barrier.srcStageMask = barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;

		auto src_extent = vk::Extent3D{out.get_image_info().extent, 1};
		for (std::uint32_t mip = 0; mip + 1 < out.get_image_info().mip_levels; ++mip) {
			vk::Extent3D dst_extent = vk::Extent3D(std::max(src_extent.width / 2, 1u), std::max(src_extent.height / 2, 1u), 1u);
			auto const src_offset = vk::Offset3D{static_cast<int>(src_extent.width), static_cast<int>(src_extent.height), 1};
			auto const dst_offset = vk::Offset3D{static_cast<int>(dst_extent.width), static_cast<int>(dst_extent.height), 1};
			blit_next_mip(mip, src_offset, dst_offset);
			src_extent = dst_extent;
		}

		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = out.get_image_info().layout;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = out.get_image_info().mip_levels;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
		barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead;
		record_barriers(command_buffer, {&barrier, 1});
	}
};

class RenderImage : public IRenderImage {
  public:
	RenderImage(RenderImage const&) = delete;
	RenderImage(RenderImage&&) = delete;
	auto operator=(RenderImage const&) = delete;
	auto operator=(RenderImage&&) = delete;

	static auto compute_mip_levels(vk::Extent2D extent) -> std::uint32_t {
		return static_cast<std::uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1u;
	}

	explicit RenderImage(NotNull<IRenderDevice*> device, CreateInfo const& create_info) : m_device(device) {
		set_info(create_info);
		if (!resize(create_info.extent)) { throw Error{"Failed to allocate Vulkan Image"}; }
	}

	~RenderImage() { defer_destroy(); }

  private:
	void defer_destroy() {
		if (!m_info.image) { return; }
		auto callback = [allocator = m_device->get_allocator(), image = m_info.image, view = std::move(m_view), allocation = m_allocation] {
			vmaDestroyImage(allocator, image, allocation);
		};
		m_device->get_defer_queue().push_callback(std::move(callback));
	}

	[[nodiscard]] auto get_image_info() const -> RenderImageInfo const& final { return m_info; }

	auto resize(vk::Extent2D extent) -> bool final {
		extent = safe_image_extent(extent);
		if (extent == m_info.extent) { return true; }

		auto create_info = m_create_info;
		create_info.extent = vk::Extent3D{extent, 1};

		auto const vici = static_cast<VkImageCreateInfo>(create_info);
		auto vaci = VmaAllocationCreateInfo{};
		vaci.usage = VMA_MEMORY_USAGE_AUTO;
		VkImage image{};
		VmaAllocation allocation{};
		if (vmaCreateImage(m_device->get_allocator(), &vici, &vaci, &image, &allocation, {}) != VK_SUCCESS) { return false; }

		auto const isr = vk::ImageSubresourceRange{m_info.aspect, 0, create_info.mipLevels, 0, create_info.arrayLayers};
		auto const make_image_view = MakeImageView{.image = image, .format = create_info.format, .subresource = isr, .type = m_info.type};
		auto view = make_image_view(m_device->get_device());
		if (!view) { return false; }

		defer_destroy();

		m_allocation = allocation;
		m_info.image = image;
		m_view = std::move(view);
		m_create_info = create_info;

		m_info.image = image;
		m_info.view = *m_view;
		m_info.extent = extent;

		return true;
	}

	void overwrite(BitmapView const bitmap, glm::ivec2 const offset) final { write_bitmaps({&bitmap, 1}, offset); }

	void write_cube(std::span<BitmapView const, 6> layers) final { write_bitmaps(layers, {}); }

	void write_bitmaps(std::span<BitmapView const> bitmaps, glm::ivec2 const offset) {
		auto const layer_count = m_info.type == vk::ImageViewType::eCube ? 6u : 1u;
		if (bitmaps.size() != layer_count) { return; }
		auto const extent_0 = bitmaps.front().extent;
		if (!std::ranges::all_of(bitmaps, [extent_0](BitmapView const& b) { return !b.bytes.empty() && b.extent == extent_0; })) { return; }

		auto const total_extent = extent_0 + offset;
		if (total_extent.x > static_cast<int>(m_info.extent.width) || total_extent.y > static_cast<int>(m_info.extent.height)) { return; }
		auto const total_bytes =
			std::accumulate(bitmaps.begin(), bitmaps.end(), std::size_t{}, [](std::size_t const a, BitmapView const& b) { return a + b.bytes.size(); });

		auto const bci = BufferCreateInfo{
			.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
			.size = total_bytes,
			.map_memory = true,
		};
		auto staging = m_device->create_buffer(bci);
		if (!staging) { return; }

		auto command_buffer = CommandBuffer{*m_device};

		auto buffer_datas = FlexArray<BufferData, 6>{};
		auto buffer_offset = vk::DeviceSize{};
		for (auto const& [index, bitmap] : std::ranges::enumerate_view(bitmaps)) {
			auto const layer = static_cast<std::uint32_t>(index);
			auto const buffer_data = BufferData{.data = bitmap.bytes.data(), .size = bitmap.bytes.size()};
			buffer_datas.insert(buffer_data);

			auto barrier = vk::ImageMemoryBarrier2{};
			barrier.image = m_info.image;
			barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = m_device->get_queue_family();
			barrier.subresourceRange = vk::ImageSubresourceRange{m_info.aspect, 0, m_info.mip_levels, layer, 1};
			barrier.oldLayout = m_info.layout;
			barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
			barrier.srcAccessMask = vk::AccessFlagBits2::eShaderSampledRead;
			barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
			barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite;
			record_barriers(command_buffer, {&barrier, 1});

			auto bic = vk::BufferImageCopy2{};
			bic.imageOffset = vk::Offset3D{offset.x, offset.y, 0};
			bic.imageExtent = vk::Extent3D{to_vk_extent(bitmap.extent), 1u};
			bic.bufferOffset = buffer_offset;
			bic.imageSubresource = vk::ImageSubresourceLayers{m_info.aspect, 0, layer, 1};
			auto const cbtii = vk::CopyBufferToImageInfo2{staging->get_buffer_info().buffer, m_info.image, vk::ImageLayout::eTransferDstOptimal, 1, &bic};
			command_buffer.get().copyBufferToImage2(cbtii);

			std::swap(barrier.oldLayout, barrier.newLayout);
			if (barrier.newLayout == vk::ImageLayout::eUndefined) { barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal; }
			std::swap(barrier.srcStageMask, barrier.dstStageMask);
			std::swap(barrier.srcAccessMask, barrier.dstAccessMask);
			record_barriers(command_buffer, {&barrier, 1});

			buffer_offset += buffer_data.size;
		}

		staging->write_sequential(buffer_datas.span());

		m_info.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		if (m_info.mip_levels > 1) { MakeMipMaps{*this, command_buffer}(m_device->get_queue_family()); }

		command_buffer.submit(*m_device);
	}

	void transition_layout(vk::CommandBuffer const command_buffer, ImageBarrier const& barrier, vk::ImageLayout const layout) final {
		if (layout == vk::ImageLayout::eUndefined) { return; }

		auto const layers = m_info.type == vk::ImageViewType::eCube ? 6u : 1u;
		auto imb = vk::ImageMemoryBarrier2{};
		imb.srcQueueFamilyIndex = imb.dstQueueFamilyIndex = m_device->get_queue_family();
		imb.image = m_info.image;
		imb.oldLayout = m_info.layout;
		imb.newLayout = layout;
		imb.srcStageMask = barrier.src_stages;
		imb.srcAccessMask = barrier.src_access;
		imb.dstStageMask = barrier.dst_stages;
		imb.dstAccessMask = barrier.dst_access;
		imb.subresourceRange = vk::ImageSubresourceRange{m_info.aspect, 0, m_info.mip_levels, 0, layers};
		record_barriers(command_buffer, {&imb, 1});

		m_info.layout = layout;
	}

	void set_info(CreateInfo const& create_info) {
		auto const mip_levels = create_info.mip_map ? compute_mip_levels(create_info.extent) : 1;

		m_create_info.usage = create_info.usage;
		m_create_info.imageType = vk::ImageType::e2D;
		m_create_info.tiling = create_info.tiling;
		if (create_info.view_type == vk::ImageViewType::eCube) {
			m_create_info.arrayLayers = 6;
			m_create_info.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
		} else {
			m_create_info.arrayLayers = 1;
		}
		m_create_info.mipLevels = mip_levels;
		m_create_info.format = create_info.format;
		m_create_info.samples = create_info.samples;

		m_info = RenderImageInfo{
			.type = create_info.view_type,
			.format = m_create_info.format,
			.layout = vk::ImageLayout::eUndefined,
			.aspect = create_info.aspect,
			.samples = create_info.samples,
			.mip_levels = mip_levels,
		};
	}

	NotNull<IRenderDevice*> m_device;

	VmaAllocation m_allocation{};

	vk::ImageCreateInfo m_create_info{};
	vk::UniqueImageView m_view{};
	RenderImageInfo m_info{};
};

class RenderBuffer : public IRenderBuffer {
  public:
	RenderBuffer(RenderBuffer const&) = delete;
	RenderBuffer(RenderBuffer&&) = delete;
	auto operator=(RenderBuffer const&) = delete;
	auto operator=(RenderBuffer&&) = delete;

	explicit RenderBuffer(NotNull<IRenderDevice*> device, CreateInfo const& create_info) : m_device(device) {
		set_info(create_info);
		if (!expand(create_info.size, create_info.map_memory)) { throw Error{"Failed to allocate Vulkan Buffer"}; }
	}

	~RenderBuffer() { defer_destroy(); }

  private:
	void defer_destroy() {
		if (!m_info.buffer) { return; }
		auto callback = [allocator = m_device->get_allocator(), buffer = m_info.buffer, allocation = m_allocation] {
			vmaDestroyBuffer(allocator, buffer, allocation);
		};
		m_device->get_defer_queue().push_callback(std::move(callback));
	}

	[[nodiscard]] auto get_buffer_info() const -> RenderBufferInfo const& final { return m_info; }

	void write_sequential(std::span<BufferData const> data) final {
		if (data.empty()) { return; }

		auto const total_size = std::accumulate(data.begin(), data.end(), std::size_t{}, [](std::size_t const a, BufferData const b) { return a + b.size; });
		expand(total_size, m_mapped != nullptr);

		if (m_mapped != nullptr) {
			// this is a host buffer, just memcpy into mapped memory.
			auto dest = std::span{static_cast<char*>(m_mapped), total_size};
			for (auto const buffer_data : data) {
				std::memcpy(dest.data(), buffer_data.data, buffer_data.size);
				dest = dest.subspan(buffer_data.size);
			}
			return;
		}

		// device buffer, need to copy from staging.
		auto const bci = BufferCreateInfo{
			.usage = vk::BufferUsageFlagBits::eTransferSrc,
			.size = total_size,
			.map_memory = true,
		};
		auto staging_buf = m_device->create_buffer(bci);
		staging_buf->set_name("staging");
		staging_buf->write_sequential(data);

		auto const bc2 = vk::BufferCopy2{0, 0, total_size};
		auto const cbi = vk::CopyBufferInfo2{staging_buf->get_buffer_info().buffer, m_info.buffer, 1, &bc2};
		auto command_buffer = CommandBuffer{*m_device};
		command_buffer.get().copyBuffer2(cbi);

		// buffer might get used on render thread next, insert a barrier here, blocking near the top of the pipe after the transfer.
		auto barrier = vk::BufferMemoryBarrier2{};
		barrier.buffer = m_info.buffer;
		barrier.size = total_size;
		barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = m_device->get_queue_family();
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eVertexInput | vk::PipelineStageFlagBits2::eIndexInput;
		barrier.dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead | vk::AccessFlagBits2::eIndexRead;
		auto di = vk::DependencyInfo{};
		di.pBufferMemoryBarriers = &barrier;
		di.bufferMemoryBarrierCount = 1;
		command_buffer.get().pipelineBarrier2(di);

		command_buffer.submit(*m_device);
	}

	void set_name(CString const name) final { vmaSetAllocationName(m_device->get_allocator(), m_allocation, name.c_str()); }

	auto expand(vk::DeviceSize size, bool const map_memory) -> bool {
		size = safe_buffer_size(size);
		if (m_info.capacity >= size) { return false; }

		auto const new_capacity = size;
		auto vaci = VmaAllocationCreateInfo{};
		vaci.usage = VMA_MEMORY_USAGE_AUTO;
		vaci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		if (map_memory) { vaci.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT; }

		auto const bci = vk::BufferCreateInfo{{}, new_capacity, m_create_info.usage};
		auto vbci = static_cast<VkBufferCreateInfo>(bci);

		VmaAllocation allocation{};
		VkBuffer buffer{};
		auto alloc_info = VmaAllocationInfo{};
		if (vmaCreateBuffer(m_device->get_allocator(), &vbci, &vaci, &buffer, &allocation, &alloc_info) != VK_SUCCESS) { return false; }

		defer_destroy();

		m_allocation = allocation;
		m_create_info = bci;
		m_info.buffer = buffer;
		m_info.capacity = bci.size;
		m_mapped = alloc_info.pMappedData;

		return true;
	}

	void set_info(CreateInfo const& create_info) {
		m_create_info.queueFamilyIndexCount = 1;
		auto const queue_family = m_device->get_queue_family();
		m_create_info.pQueueFamilyIndices = &queue_family;
		m_create_info.usage = create_info.usage;
		m_create_info.sharingMode = vk::SharingMode::eExclusive;
		if (!create_info.map_memory) { m_create_info.usage |= vk::BufferUsageFlagBits::eTransferDst; }
		m_info = RenderBufferInfo{
			.usage = m_create_info.usage,
		};
	}

	NotNull<IRenderDevice*> m_device;

	VmaAllocation m_allocation{};

	vk::BufferCreateInfo m_create_info{};
	RenderBufferInfo m_info{};
	void* m_mapped{};
};
} // namespace levk
