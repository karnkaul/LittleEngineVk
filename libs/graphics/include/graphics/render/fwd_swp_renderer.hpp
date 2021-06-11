#pragma once
#include <graphics/render/renderer.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
// Forward, Swapchain
class RendererFS : public ARenderer {
  public:
	RendererFS(not_null<Swapchain*> swapchain, u8 buffering = 2);
	~RendererFS() override;

	Technique technique() const noexcept override { return {.approach = Approach::eForward, .target = Target::eSwapchain}; }
	RenderPasses renderPasses() const noexcept override { return {m_storage.renderPass}; }

	std::optional<Draw> beginFrame(CommandBuffer::PassInfo const& info) override;
	bool endFrame() override;

  private:
	struct Buf {
		Command command;
		RenderSemaphore semaphore;
		vk::Framebuffer framebuffer;
	};
	struct Storage {
		RingBuffer<Buf> buf;
		vk::RenderPass renderPass;
	};
	struct {
		Attachment colour = {{vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR}, {}};
		Attachment depth = {{vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal}, {}};
	} m_attachments;

	Storage make() const;
	void destroy();

	Storage m_storage;
};
} // namespace le::graphics
