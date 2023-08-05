#include <le/graphics/image_barrier.hpp>
#include <le/graphics/resource.hpp>

namespace le::graphics {
ImageBarrier::ImageBarrier(vk::Image image, std::uint32_t mip_levels, std::uint32_t array_layers) {
	barrier.image = image;
	barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, array_layers};
}

ImageBarrier::ImageBarrier(Image const& image)
	: ImageBarrier(image.image(), image.mip_levels(), image.view_type() == vk::ImageViewType::eCube ? Image::cubemap_layers_v : 1) {}

auto ImageBarrier::set_full_barrier(vk::ImageLayout src, vk::ImageLayout dst) -> ImageBarrier& {
	barrier.oldLayout = src;
	barrier.newLayout = dst;
	barrier.srcStageMask = barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
	barrier.srcAccessMask = barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
	return *this;
}

auto ImageBarrier::set_undef_to_optimal(bool depth) -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eAttachmentOptimal;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
	barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
	if (depth) {
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	} else {
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
	}
	return *this;
}

auto ImageBarrier::set_undef_to_transfer_dst() -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
	barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
	barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eTransferRead;
	return *this;
}

auto ImageBarrier::set_optimal_to_read_only(bool depth) -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eAttachmentOptimal;
	barrier.newLayout = vk::ImageLayout::eReadOnlyOptimal;
	if (depth) {
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	} else {
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
	}
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
	return *this;
}

auto ImageBarrier::set_optimal_to_transfer_src() -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eAttachmentOptimal;
	barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
	barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite;
	return *this;
}

auto ImageBarrier::set_optimal_to_present() -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eAttachmentOptimal;
	barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
	barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
	return *this;
}

auto ImageBarrier::set_transfer_dst_to_optimal(bool depth) -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eAttachmentOptimal;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eTransferRead;
	if (depth) {
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
		barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	} else {
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
	}
	return *this;
}

auto ImageBarrier::set_transfer_dst_to_present() -> ImageBarrier& {
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eTransferRead;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
	barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
	return *this;
}

auto ImageBarrier::transition(vk::CommandBuffer cmd) const -> void {
	if (!barrier.image) { return; }
	transition(cmd, {&barrier, 1u});
}

auto ImageBarrier::transition(vk::CommandBuffer cmd, std::span<vk::ImageMemoryBarrier2 const> barriers) -> void {
	if (barriers.empty()) { return; }
	auto vdi = vk::DependencyInfo{};
	vdi.imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size());
	vdi.pImageMemoryBarriers = barriers.data();
	cmd.pipelineBarrier2(vdi);
}
} // namespace le::graphics
