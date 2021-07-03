#pragma once
#include <graphics/render/renderer.hpp>

namespace le::graphics {
// Forward, Swapchain
class RendererFwdSwp : public ARenderer {
  public:
	RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	Tech tech() const noexcept override { return {Transition::eCommandBuffer, Approach::eForward, Target::eSwapchain}; }

	vk::RenderPass renderPass3D() const noexcept override { return *m_storage.renderPass; }
	vk::RenderPass renderPassUI() const noexcept override { return *m_storage.renderPass; }

	std::optional<Draw> beginFrame() override;
	void endDraw(RenderTarget const& target) override;
};
} // namespace le::graphics
