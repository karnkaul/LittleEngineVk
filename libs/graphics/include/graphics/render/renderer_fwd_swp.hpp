#pragma once
#include <graphics/render/renderer.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
struct ImageMaker {
	Image::CreateInfo info;
	not_null<VRAM*> vram;

	ImageMaker(not_null<VRAM*> vram) noexcept : vram(vram) {}

	static bool ready(std::optional<Image>& out, Extent2D extent) noexcept { return out && cast(out->extent()) == extent; }

	Image make(Extent2D extent) {
		info.createInfo.extent = vk::Extent3D(extent.x, extent.y, 1);
		return Image(vram, info);
	}

	Image& refresh(std::optional<Image>& out, Extent2D extent) {
		if (!ready(out, extent)) { out = make(extent); }
		return *out;
	}
};

// Forward, Swapchain
class RendererFwdSwp : public ARenderer {
  public:
	RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering = doubleBuffer);

	Technique technique() const noexcept override { return {.approach = Approach::eForward, .target = Target::eSwapchain}; }
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
