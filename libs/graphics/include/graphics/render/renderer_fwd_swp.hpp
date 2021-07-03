#pragma once
#include <graphics/render/renderer.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
// Forward, Swapchain
class RendererFwdSwp : public ARenderer {
  public:
	RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	Tech tech() const noexcept override { return {Approach::eForward, Target::eOffScreen}; }

	vk::RenderPass renderPass3D() const noexcept override { return *m_storage.renderPass; }
	vk::RenderPass renderPassUI() const noexcept override { return *m_storage.renderPass; }

	std::optional<Draw> beginFrame() override;
	void beginDraw(RenderTarget const& target, FrameDrawer& drawer, ScreenView const& view, RGBA clear, vk::ClearDepthStencilValue depth = {1.0, 1}) override;
	void endDraw(RenderTarget const& target) override;
	void endFrame() override;
	bool submitFrame() override;

  private:
	struct Buf : Cmd {
		using Cmd::Cmd;
		std::optional<Image> offscreen;
		Deferred<vk::Framebuffer> framebuffer;
	};
	struct Storage {
		RingBuffer<Buf> buf;
		Deferred<vk::RenderPass> renderPass;
		RenderImage swapImg;
	};

	Storage make() const;

	Storage m_storage;
	ImageMaker m_imgMaker;
};
} // namespace le::graphics
