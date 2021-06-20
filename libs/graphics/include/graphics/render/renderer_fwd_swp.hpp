#pragma once
#include <graphics/render/renderer.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
// Forward, Swapchain
class RendererFwdSwp : public ARenderer {
  public:
	RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	Technique technique() const noexcept override { return {.approach = Approach::eForward, .target = Target::eSwapchain}; }
	vk::RenderPass renderPass3D() const noexcept override { return *m_storage.renderPass; }
	vk::RenderPass renderPassUI() const noexcept override { return *m_storage.renderPass; }

	std::optional<Draw> beginFrame() override;
	void beginDraw(RenderTarget const& target, FrameDrawer& drawer, RGBA clear, vk::ClearDepthStencilValue depth = {1.0, 1}) override;
	void endDraw(RenderTarget const& target) override;
	void endFrame() override;
	bool submitFrame() override;

  private:
	struct Buf : Cmd {
		using Cmd::Cmd;
		Deferred<vk::Framebuffer> framebuffer;
	};
	struct Storage {
		RingBuffer<Buf> buf;
		Deferred<vk::RenderPass> renderPass;
	};

	Storage make() const;

	Storage m_storage;
};
} // namespace le::graphics
