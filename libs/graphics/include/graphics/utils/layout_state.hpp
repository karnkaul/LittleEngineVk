#pragma once
#include <core/not_null.hpp>
#include <graphics/common.hpp>
#include <ktl/tmutex.hpp>
#include <unordered_map>

namespace le::graphics {
class CommandBuffer;

using StageAccess = TPair<vk::PipelineStageFlags, vk::AccessFlags>;

struct LayoutStages {
	static constexpr vk::ImageAspectFlags af_colour_v = vIAFB::eColor;
	static constexpr vk::ImageAspectFlags af_depth_v = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

	static constexpr StageAccess sa_top_v = {vPSFB::eTopOfPipe, {}};
	static constexpr StageAccess sa_bottom_v = {vPSFB::eBottomOfPipe, {}};
	static constexpr StageAccess sa_all_commands_v = {vPSFB::eAllCommands, {}};
	static constexpr StageAccess sa_depth_v = {vPSFB::eEarlyFragmentTests | vPSFB::eLateFragmentTests, vAFB::eDepthStencilAttachmentWrite};
	static constexpr StageAccess sa_colour_v = {vPSFB::eColorAttachmentOutput, vAFB::eColorAttachmentWrite};
	static constexpr StageAccess sa_transfer_v = {vPSFB::eTransfer, vAFB::eTransferWrite | vAFB::eTransferRead};

	StageAccess src;
	StageAccess dst;
	vk::ImageAspectFlags aspects;

	static constexpr LayoutStages allCommands(vk::ImageAspectFlags aspects = af_colour_v) noexcept { return {sa_all_commands_v, sa_all_commands_v, aspects}; }
	static constexpr LayoutStages topBottom(vk::ImageAspectFlags aspects = af_colour_v) noexcept { return {sa_top_v, sa_bottom_v, aspects}; }
	static constexpr LayoutStages topColour(vk::ImageAspectFlags aspects = af_colour_v) noexcept { return {sa_top_v, sa_colour_v, aspects}; }
	static constexpr LayoutStages colourTransfer(vk::ImageAspectFlags aspects = af_colour_v) noexcept { return {sa_colour_v, sa_transfer_v, aspects}; }
	static constexpr LayoutStages transferBottom(vk::ImageAspectFlags aspects = af_colour_v) noexcept { return {sa_transfer_v, sa_bottom_v, aspects}; }
	static constexpr LayoutStages topDepth(vk::ImageAspectFlags aspects = af_depth_v) noexcept { return {sa_top_v, sa_depth_v, aspects}; }
};

class LayoutState {
  public:
	vk::ImageLayout get(vk::Image img) const;
	void transition(CommandBuffer cb, vk::Image img, vk::ImageLayout layout, LayoutStages const& ls, u32 layers = 1U, u32 mips = 1U);
	void clear() { ktl::tlock(m_map)->clear(); }

	void presented(vk::Image image) { force(image, vk::ImageLayout::ePresentSrcKHR); }
	void drawn(vk::Image depth) { force(depth, vk::ImageLayout::eUndefined); }
	void force(vk::Image image, vk::ImageLayout layout) { (*ktl::tlock(m_map))[image] = layout; }

	template <typename LT>
	void transition(CommandBuffer const& cb, vk::Image image, vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor) {
		transition(cb, image, aspect, LT::layout, LT::src, LT::dst);
	}

  private:
	ktl::strict_tmutex<std::unordered_map<vk::Image, vk::ImageLayout>> m_map;
};
} // namespace le::graphics
