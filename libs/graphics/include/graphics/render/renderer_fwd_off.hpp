#pragma once
#include <graphics/render/renderer.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
// Forward, OffScreen
class RendererFwdOff : public ARenderer {
  public:
	RendererFwdOff(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	Tech tech() const noexcept override { return {Transition::eCommandBuffer, Approach::eForward, Target::eOffScreen}; }

	vk::RenderPass renderPass3D() const noexcept override { return *m_storage.renderPass; }
	vk::RenderPass renderPassUI() const noexcept override { return *m_storage.renderPass; }

	std::optional<Draw> beginFrame() override;
	void endDraw(RenderTarget const& target) override;

  private:
	RenderImage m_swapchainImage;
	vk::Format m_colourFormat = vk::Format::eR8G8B8A8Unorm;
	std::size_t m_colourIndex = 0;
};
} // namespace le::graphics
