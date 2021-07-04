#pragma once
#include <unordered_map>
#include <core/not_null.hpp>
#include <graphics/common.hpp>

namespace le::graphics {
class CommandBuffer;

using StageAccess = TPair<vk::PipelineStageFlags, vk::AccessFlags>;

constexpr StageAccess topOfPipe = {vPSFB::eTopOfPipe, {}};
constexpr StageAccess bottomOfPipe = {vPSFB::eBottomOfPipe, {}};
constexpr StageAccess depthWrite = {vPSFB::eEarlyFragmentTests | vPSFB::eLateFragmentTests, vAFB::eDepthStencilAttachmentWrite};
constexpr StageAccess colourWrite = {vPSFB::eColorAttachmentOutput, vAFB::eColorAttachmentWrite};
constexpr vk::ImageAspectFlags depthStencil = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

class LayoutState {
  public:
	vk::ImageLayout get(vk::Image img) const;
	void transition(vk::Image img, vk::ImageAspectFlags aspect, CommandBuffer cb, vk::ImageLayout layout, StageAccess src, StageAccess dst);
	void clear() { m_map.clear(); }

	void presented(vk::Image image) { force(image, vk::ImageLayout::ePresentSrcKHR); }
	void drawn(vk::Image depth) { force(depth, vk::ImageLayout::eUndefined); }
	void force(vk::Image image, vk::ImageLayout layout) { m_map[image] = layout; }

	template <typename LT>
	void transition(CommandBuffer const& cb, vk::Image image, vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor) {
		transition(image, aspect, cb, LT::layout, LT::src, LT::dst);
	}

  private:
	std::unordered_map<vk::Image, vk::ImageLayout> m_map;
};

namespace lt {
namespace partial {
struct Transfer {
	static constexpr StageAccess src = {vPSFB::eColorAttachmentOutput, vAFB::eColorAttachmentWrite};
	static constexpr StageAccess dst = {vPSFB::eTransfer, vAFB::eTransferWrite | vAFB::eTransferRead};
};
struct Present {
	static constexpr vk::ImageLayout layout = vk::ImageLayout::ePresentSrcKHR;
	static constexpr StageAccess dst = {vPSFB::eBottomOfPipe, {}};
};
} // namespace partial
struct ColourWrite {
	static constexpr vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal;
	static constexpr StageAccess src = {vPSFB::eTopOfPipe, {}};
	static constexpr StageAccess dst = {vPSFB::eColorAttachmentOutput, vAFB::eColorAttachmentWrite};
};
struct DepthStencilWrite {
	static constexpr vk::ImageLayout layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	static constexpr StageAccess src = {vPSFB::eTopOfPipe, {}};
	static constexpr StageAccess dst = {vPSFB::eEarlyFragmentTests | vPSFB::eLateFragmentTests, vAFB::eDepthStencilAttachmentWrite};
};
struct TransferSrc : partial::Transfer {
	static constexpr vk::ImageLayout layout = vk::ImageLayout::eTransferSrcOptimal;
};
struct TransferDst : partial::Transfer {
	static constexpr vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal;
};
struct DrawPresent : partial::Present {
	static constexpr StageAccess src = {vPSFB::eColorAttachmentOutput, vAFB::eColorAttachmentWrite};
};
struct TransferPresent : partial::Present {
	static constexpr StageAccess src = {vPSFB::eTransfer, vAFB::eTransferWrite | vAFB::eTransferRead};
};
} // namespace lt
} // namespace le::graphics
