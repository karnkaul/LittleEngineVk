#pragma once
#include <unordered_map>
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
	struct Img {
		vk::Image image;
		vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
	};

	vk::ImageLayout get(vk::Image img) const;
	void transition(Img img, CommandBuffer cb, vk::ImageLayout layout, StageAccess src, StageAccess dst);
	void clear() { m_map.clear(); }

	void presented(vk::Image image) { force(image, vk::ImageLayout::ePresentSrcKHR); }
	void drawn(vk::Image depth) { force(depth, vk::ImageLayout::eUndefined); }
	void force(vk::Image image, vk::ImageLayout layout) { m_map[image] = layout; }

  private:
	std::unordered_map<vk::Image, vk::ImageLayout> m_map;
};
} // namespace le::graphics
